<?php
// $Id$

/**
 * @file
 * WSClient UI - implements service description management and configuration
 * screens.
 */

/**
 * Provides a form to add, edit and clone web service descriptions.
 */
function wsclient_service_form($form, &$form_state, $service, $op = 'edit') {
  if ($op == 'clone') {
    $service->label .= ' (cloned)';
    $service->name .= '_clone';
  }
  $type_info = wsclient_get_types();
  if (empty($type_info)) {
    drupal_set_message(t('No service types were found, please enable a module that provides a service type.'), 'warning');
  }
  $types = array();
  foreach ($type_info as $type => $info) {
    $types[$type] = $info['label'];
  }

  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => 'Label',
    '#default_value' => $service->label,
    '#required' => TRUE,
    '#description' => t('The human-readable name.'),
  );
  $form['name'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($service->name) ? $service->name : '',
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => 'wsclient_service_load',
      'source' => array('label'),
    ),
    '#required' => TRUE,
    '#description' => t('The machine-readable name of this service is used internally to identify the service. This name must contain only lowercase letters, numbers, and underscores and must be unique.'),
    '#element_validate' => array('form_validate_machine_name', 'entity_ui_validate_machine_name'),
  );
  $form['url'] = array(
    '#type' => 'textfield',
    '#title' => 'URL',
    '#default_value' => $service->url,
    '#required' => TRUE,
    '#description' => t('The URL of the web service.'),
    '#element_validate' => array('wsclient_ui_element_url_validate'),
  );
  $form['type'] = array(
    '#type' => 'select',
    '#title' => 'Type',
    '#default_value' => $service->type,
    '#options' => $types,
    '#required' => TRUE,
    '#description' => t('The type of the web service.'),
  );
  $rows = array();
  foreach ($service->operations as $name => $operation) {
    $row = array();
    $row[] = l($operation['label'], WSCLIENT_UI_PATH . '/manage/' . $service->name . '/edit/' . $name);
    $row[] = l(t('Edit'), WSCLIENT_UI_PATH . '/manage/' . $service->name . '/edit/' . $name);
    $row[] = l(t('Clone'), WSCLIENT_UI_PATH . '/manage/' . $service->name . '/clone/' . $name);
    $row[] = l(t('Delete'), WSCLIENT_UI_PATH . '/manage/' . $service->name . '/delete/' . $name);
    $rows[] = $row;
  }
  $header = array(t('Label'), array('data' => t('Operations'), 'colspan' => 3));
  $form['operations'] = array(
    '#access' => TRUE,
    '#tree' => TRUE,
    '#theme' => 'table',
    '#empty' => t('None'),
    '#caption' => t('Operations'),
    '#rows' => $rows,
    '#header' => $header,
  );
  $form['add_operaton'] = array(
    '#theme' => 'links__wsclient',
    '#links' => array(
      'add_op' => array(
        'title' => t('Add operation'),
        'href' => WSCLIENT_UI_PATH . '/manage/' . $service->name . '/add/operation',
      ),
    ),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Submit callback of web service description form.
 */
function wsclient_service_form_submit($form, &$form_state) {
  $service = entity_ui_form_submit_build_entity($form, $form_state);
  $service->save();
  $form_state['redirect'] = WSCLIENT_UI_PATH;
}

/**
 * FAPI callback to validate a URL.
 */
function wsclient_ui_element_url_validate($element, &$form_state) {
  if (!valid_url($element['#value'], TRUE)) {
    form_error($element, t('Please enter a valid URL.'));
  }
}

/**
 * Form to add operations.
 */
function wsclient_ui_add_operation($form, &$form_state, $service) {
  dpm($service);
  $form['type'] = array(
    '#type' => 'select',
    '#title' => 'Type',
    '#default_value' => 1,
    '#options' => array(1, 2, 3),
    '#required' => TRUE,
    '#description' => t('The type of the web service.'),
  );
  return $form;
}
