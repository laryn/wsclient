<?php
// $Id$

/**
 * @file
 * Web service client - Rules integration.
 */

/**
 * Implements hook_rules_event_info().
 */
function wsclient_rules_event_info() {
  $events = array();
  $valid_types = rules_fetch_data('data_info');

  foreach (entity_load('wsclient_service', FALSE) as $name => $service) {
    $service_types = $service->entities() + $service->dataTypes();
    foreach ($service->events() as $event => $event_info) {
      $is_valid = array_diff_key(array_flip(array('label', 'variables')), $event_info) == array();
      // Don't add in events, that stem from another site.
      if (strpos($event, 'wsclient_') !== 0 && $is_valid) {
        // Transform data types of the variables
        foreach ($event_info['variables'] as $var_name => &$var_info) {
          // Make sure the info is complete and contains known types.
          if (array_diff_key(array_flip(array('label', 'type')), $var_info) !== array()) {
            continue 2;
          }
          // Map the type name of types defined by the remote.
          $var_info['type'] = _wsclient_map_type($name, $service_types, $var_info['type']);

          if (!isset($valid_types[$var_info['type']])) {
            continue 2;
          }
          // We don't support loading via handler for remote variables.
          unset($var_info['handler']);
        }
        $event_info['group'] = t('Web service @label', array('@label' => $service->label));
        // Add some info so we can easily get the remote for an event later on.
        $event_info['wsclient']['remote'] = $name;
        $event_info['wsclient']['event'] = $event;
        $events['wsclient_' . $name . '_' . $event] = $event_info;
      }
    }
  }
  return $events;
}

/**
 * Implements hook_rules_data_info().
 */
function wsclient_rules_data_info() {
  $types = array();
  foreach (entity_load('wsclient_service', FALSE) as $name => $service) {
    foreach ($service->dataTypes() as $type => $type_info) {
      $type_info = array_intersect_key($type_info, array_flip(array('label', 'property info'))) + array(
        'wrap' => TRUE,
        'label' => t('Web service %label %type', array('%label' => $service->label, '%type' => $type)),
        'creation callback' => 'rules_action_data_create_array',
      );
      $types['wsclient_' . $name . '_' . $type] = $type_info;
    }
  }
  return $types;
}

/**
 * Implements hook_rules_action_info().
 */
function wsclient_rules_action_info() {
  $return = array();
  foreach (entity_load('wsclient_service', FALSE) as $name => $service) {
    $service_types = $service->entities() + $service->dataTypes();
    foreach ($service->actions() as $item_name => $info) {
      $info += array('parameter' => array(), 'provides' => array());
      // Map the types.
      foreach ($info['parameter'] as $param => &$param_info) {
        $param_info['type'] = _wsclient_map_type($name, $service_types, $param_info['type']);
      }
      foreach ($info['provides'] as $var_name => &$var_info) {
        $var_info['type'] = _wsclient_map_type($name, $service_types, $var_info['type']);
      }
      $return['wsclient_' . $name . '_' . $item_name] = $info;
    }
  }
  return $return;
}

/**
 * Maps the type name from the name used by the remote info to the rules name.
 */
function _wsclient_map_type($name, $service_types, $type) {
  if (is_array($type)) {
    foreach ($type as $key => $entry) {
      $type[$key] = _wsclient_map_type($name, $service_types, $entry);
    }
  }
  elseif (isset($service_types[$type])) {
    $type = 'wsclient_' . $name . '_' . $type;
  }
  return $type;
}

