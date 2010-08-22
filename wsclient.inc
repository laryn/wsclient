<?php
// $Id$

/**
 * @file
 * Web service client - include file.
 */

/**
 * Class representing web service descriptions.
 */
class WSClientServiceDescription extends EntityDBExtendable {

  public $settings = array();
  public $name;
  public $label;
  public $url;
  public $token;

  protected $endpoint = NULL;
  protected $info = array();
  protected $subscriptions = array();

  function __construct($values = array(), $entity_type = 'wsclient_service') {
    parent::__construct($values, $entity_type);
    // Make sure there is a token for communicating with the remote.
    if (!isset($this->token)) {
      $this->token = drupal_get_token(time());
    }
  }

  /**
   * Determines access to the web service.
   */
  public function access($account = NULL) {
    if (method_exists($this->endpoint(), 'access')) {
      return $this->endpoint()->access($account);
    }
    return user_access('interact with ' . $this->name, $account);
  }

  /**
   * Returns the associated web service endpoint object.
   *
   * @return WSClientEndpointInterface
   */
  public function endpoint() {
    if (!isset($this->endpoint)) {
      $types = wsclient_get_types();
      $this->endpoint = new $types[$this->type]['class']($this);
    }
    return $this->endpoint;
  }

  /**
   * Loads the entity of the given type and id. In case of errors a
   * RulesException is thrown.
   */
  public function load($type, $id) {
    try {
      return $this->endpoint()->load($type, $id);
    }
    catch (Exception $e) {
      $args = array('%name' => $this->name, '%message' => $e->getMessage(), '%type' => $type, '%id' => print_r($id,1));
      throw new WSClientException('Unable to load %id of type %type from the web service description %name. Error message: %message', $args);
    }
  }

  /**
   * Returns info about events of the web service.
   */
  public function events() {
    if (empty($this->info)) {
      $this->refreshInfo();
    }
    return !empty($this->info['events']) ? $this->info['events'] : array();
  }

  /**
   * Returns info about the entities of the web service.
   */
  public function entities() {
    if (empty($this->info)) {
      $this->refreshInfo();
    }
    return !empty($this->info['entities']) ? $this->info['entities'] : array();
  }

  /**
   * Returns info about the data types of the web service, thus types being not
   * entities.
   */
  public function dataTypes() {
    if (empty($this->info)) {
      $this->refreshInfo();
    }
    return !empty($this->info['dataTypes']) ? $this->info['dataTypes'] : array();
  }

  /**
   * Returns info about the actions of the web service.
   */
  public function actions() {
    if (empty($this->info)) {
      $this->refreshInfo();
    }
    return !empty($this->info['actions']) ? $this->info['actions'] : array();
  }

  /**
   * Returns info about the conditions of the web service.
   */
  public function conditions() {
    if (empty($this->info)) {
      $this->refreshInfo();
    }
    return !empty($this->info['conditions']) ? $this->info['conditions'] : array();
  }

  protected function refreshInfo() {
    try {
      $this->info['entities'] = $this->endpoint()->entities();
      $this->info['dataTypes'] = $this->endpoint()->dataTypes();
      $this->info['actions'] = $this->endpoint()->actions();
      $this->info['conditions'] = $this->endpoint()->conditions();
      $this->info['events'] = $this->endpoint()->events();
      $this->storeInfo();
    }
    catch (Exception $e) {
      $args = array('%name' => $this->name, '%message' => $e->getMessage());
      watchdog('wsclient', 'Error getting definitions from the web service %name. Error message: %message.', $args, WATCHDOG_ALERT);
    }
  }

  public function clearCache() {
    $this->refreshInfo();
  }

  public function isSubscribedTo($event) {
    return isset($this->subscriptions[$event]);
  }

  /**
   * Subscribes to the given event, so we get event notitifcations whenever the
   * event occurs.
   */
  public function subscribe($event) {
    // Make sure the info is retrieved and thus the token is stored, so the
    // token won't change in future. $this->events() does that for us.
    if (($events = $this->events()) && isset($events[$event])) {
      try {
        $this->endpoint()->subscribe($event);
        $this->subscriptions[$event] = TRUE;
        db_merge('wsclient_subscriptions')
          ->key(array(
            'name' => $this->name,
            'event' => $event,
          ))
          ->execute();
      }
      catch (Exception $e) {
        $args = array('%event' => $event, '%name' => $this->name, '%message' => $e->getMessage());
        watchdog('wsclient', 'Error subscribing to event %event of web service %name. Error message: %message.', $args, WATCHDOG_ERROR);
      }
    }
  }

  /**
   * Unsubscribes from the given event.
   */
  public function unsubscribe($event) {
    try {
      $this->endpoint()->unsubscribe($event);
      unset($this->subscriptions[$event]);
      db_delete('wsclient_subscriptions')
        ->condition('name', $this->name)
        ->condition('event', $event)
        ->execute();
    }
    catch (Exception $e) {
      $args = array('%event' => $event, '%name' => $this->name, '%message' => $e->getMessage());
      watchdog('wsclient', 'Error unsubscribing from event %event of web service %name. Error message: %message.', $args, WATCHDOG_ERROR);
    }
  }

  /**
   * Stores the token and remote metadata independent from the web service
   * object itself, so a web service may live in code via the default hook but
   * still store this information.
   */
  protected function storeInfo() {
    db_merge('wsclient_info')
      ->key(array('name' => $this->name))
      ->fields(array(
        'info' => serialize($this->info),
        'token' => $this->token,
      ))
      ->execute();
  }

  public function save() {
    parent::save();
    $this->storeInfo();
  }

  /**
   * Loads the stored info for the given web service objects.
   */
  public static function attachLoad($services) {
    $result = db_select('wsclient_info', 'r')
    ->fields('r')
    ->condition('name', array(array_keys($services)))
    ->execute();

    foreach ($result as $record) {
      $services[$record->name]->info = unserialize($record->info);
      $services[$record->name]->token = $record->token;
    }

    // Load subscribed events.
    $result = db_select('wsclient_subscriptions', 'r')
    ->fields('r')
    ->condition('name', array(array_keys($services)))
    ->execute();
    foreach ($result as $record) {
      $services[$record->name]->subscriptions[$record->event] = TRUE;
    }
  }
}

/**
 * Implements hook_wsclient_service_load().
 */
function wsclient_wsclient_service_load($services) {
  WSClientServiceDescription::attachLoad($services);
}

/**
 * Implements hook_default_wsclient_service_alter().
 */
function wsclient_default_wsclient_service_alter($services) {
  WSClientServiceDescription::attachLoad($services);
}

/**
 * Interface for remote endpoints. In case of any errors the implementing
 * classes should throw exceptions.
 */
interface WSClientEndpointInterface {

  public function __construct(WSClientServiceDescription $service);

  /**
   * Load remote data.
   */
  public function load($type, $id);

  /**
   * Connect to the remote web service and execute an operation.
   *
   * @param string $operation
   *   The name of the operation.
   * @param $arguments
   *   The arguments needed for the operation.
   *
   * @return
   *   The response of the web service call.
   */
  public function executeOperation($operation, $arguments);

  /**
   * An array of definitions for the provided events.
   */
  public function events();

  /**
   * Subscribe to a remote event.
   */
  public function subscribe($event);

  /**
   * Unsubscribe from a remote event.
   */
  public function unsubscribe($event);

  /**
   * An array of info about entity types used by the provided
   * events/conditions/actions.
   */
  public function entities();

  /**
   * An array of info about data types used by the provided events/conditions/
   * actions being not entities.
   */
  public function dataTypes();

  /**
   * An array of definitions for the provided actions.
   */
  public function actions();

  /**
   * An array of definitions for the provided conditions.
   */
  public function conditions();

  /**
   * Allows altering the configuration form of web service definitions, such
   * that the form can include endpoint type specific configuration settings.
   */
  public function formAlter(&$form, &$form_state);

}

/**
 * A controller for loading web service descriptions.
 */
class WSClientServiceEntityController extends DrupalDefaultEntityController {

  protected $service;

  function __construct($entityType) {
    parent::__construct($entityType);
    $this->service = wsclient_service_load($this->entityInfo['wsclient']['remote']);
  }


  /**
   * Override load to fetch the data from the web service. For now we don't
   * support using $conditions or revisions.
   */
  public function load($ids = array(), $conditions = array()) {
    $entities = array();

    // Create a new variable which is either a prepared version of the $ids
    // array for later comparison with the entity cache, or FALSE if no $ids
    // were passed. The $ids array is reduced as items are loaded from cache,
    // and we need to know if it's empty for this reason to avoid querying the
    // database when all requested entities are loaded from cache.
    $passed_ids = !empty($ids) ? array_flip($ids) : FALSE;
    // Try to load entities from the static cache, if the entity type supports
    // static caching.
    if ($this->cache) {
      $entities += $this->cacheGet($ids, $conditions);
      // If any entities were loaded, remove them from the ids still to load.
      if ($passed_ids) {
        $ids = array_keys(array_diff_key($passed_ids, $entities));
      }
    }

    // Load any remaining entities from the web service.
    if ($ids === FALSE || $ids) {
      foreach ($ids as $id) {
        $queried_entities[$id] = $this->service->load($this->entityInfo['wsclient']['type'], $id);
      }
    }

    // Pass all entities loaded through $this->attachLoad(),
    // which attaches fields (if supported by the entity type) and calls the
    // entity type specific load callback, for example hook_node_load().
    if (!empty($queried_entities)) {
      $this->attachLoad($queried_entities);
      $entities += $queried_entities;
    }

    if ($this->cache && !empty($queried_entities)) {
      $this->cacheSet($queried_entities);
    }

    // Ensure that the returned array is ordered the same as the original
    // $ids array if this was passed in and remove any invalid ids.
    if ($passed_ids) {
      // Remove any invalid ids from the array.
      $passed_ids = array_intersect_key($passed_ids, $entities);
      foreach ($entities as $entity) {
        $passed_ids[$entity->{$this->idKey}] = $entity;
      }
      $entities = $passed_ids;
    }

    return $entities;
  }
}

/**
 * Custom exception class to enhance default PHP exceptions.
 */
class WSClientException extends Exception {

  public $msg, $args;

  /**
   * @param $msg
   *   The exception message containing placeholder as t().
   * @param $args
   *   Replacement arguments such as for t().
   */
  function __construct($msg, $args = array()) {
    $this->msg = $msg;
    $this->args = $args;
    unset($this->message);
  }

  /**
   * The getMessage() method cannot be overriden, so this is the only
   * possibility to override its output.
   */
  public function __get($name) {
    if ($name == 'message') {
      return $this->__toString();
    }
    return parent::$name;
  }

  public function __toString() {
    return t($this->msg, $this->args);
  }
}
