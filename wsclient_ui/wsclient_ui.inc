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
  if ($op == 'edit') {
    // Operations of the web service in a table
    $rows = array();
    $operations = wsclient_ui_label_sort($service->operations);
    foreach ($operations as $name => $operation) {
      $row = array();
      $row[] = l($operation['label'], WSCLIENT_UI_PATH . '/manage/' . $service->name . '/operation/' . $name);
      $row[] = l(t('Edit'), WSCLIENT_UI_PATH . '/manage/' . $service->name . '/operation/' . $name);
      $row[] = l(t('Delete'), WSCLIENT_UI_PATH . '/manage/' . $service->name . '/operation/' . $name . '/delete');
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
    // Data types of the web service in a table
    $rows = array();
    $datatypes = wsclient_ui_label_sort($service->datatypes);
    foreach ($datatypes as $name => $datatype) {
      $row = array();
      $row[] = l($datatype['label'], WSCLIENT_UI_PATH . '/manage/' . $service->name . '/type/' . $name);
      $row[] = l(t('Edit'), WSCLIENT_UI_PATH . '/manage/' . $service->name . '/type/' . $name);
      $row[] = l(t('Delete'), WSCLIENT_UI_PATH . '/manage/' . $service->name . '/type/' . $name . '/delete');
      $rows[] = $row;
    }
    $header = array(t('Label'), array('data' => t('Operations'), 'colspan' => 3));
    $form['datatypess'] = array(
      '#access' => TRUE,
      '#tree' => TRUE,
      '#theme' => 'table',
      '#empty' => t('None'),
      '#caption' => t('Data types'),
      '#rows' => $rows,
      '#header' => $header,
    );
    $form['add_type'] = array(
      '#theme' => 'links__wsclient',
      '#links' => array(
        'add_op' => array(
          'title' => t('Add data type'),
          'href' => WSCLIENT_UI_PATH . '/manage/' . $service->name . '/add/type',
        ),
      ),
    );
  }
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
  if ($form_state['op'] == 'add') {
    $form_state['redirect'] = WSCLIENT_UI_PATH . '/manage/' . $service->name;
  }
  else {
    $form_state['redirect'] = WSCLIENT_UI_PATH;
  }
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
 * Operation form.
 */
function wsclient_ui_operation($form, &$form_state, $service, $operation, $op = 'edit') {
  if ($op == 'add') {
    $operation = array(
      'name' => '',
      'parameter' => array(),
    );
  }
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => isset($operation['label']) ? $operation['label'] : '',
    '#required' => TRUE,
    '#description' => t('The human-readable name of the operation.'),
  );
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#default_value' => isset($operation['name']) ? $operation['name'] : '',
    '#required' => TRUE,
    '#description' => t('The machine-readable name of this operation is used internally to identify the operation.'),
    '#element_validate' => array('wsclient_ui_operation_name_validate'),
  );
  $form['parameters'] = array(
    '#tree' => TRUE,
    '#element_validate' => array('wsclient_ui_validate_parameters'),
    '#theme' => 'wsclient_ui_variable_form',
    '#title' => t('Parameters'),
    '#description' => t('Specify the parameters for the operation. For each parameter you have to specify a certain data type and a unique name containing only alphanumeric characters and underscores.'),
  );
  $weight = 0;
  $types = wsclient_ui_types();
  if (isset($operation['parameter'])) {
    foreach ($operation['parameter'] as $name => $info) {
      $form['parameters']['items'][$name] = _wsclient_ui_parameter_row($service, $types, $name, $info);
      $form['parameters']['items'][$name]['weight']['#default_value'] = $weight++;
    }
  }
  // Always add three empty lines.
  $form_state['more'] = isset($form_state['more']) ? $form_state['more'] : 3;
  for ($i = 0; $i < $form_state['more']; $i++) {
    $form['parameters']['items'][$i] = _wsclient_ui_parameter_row($service, $types);
    $form['parameters']['items'][$i]['weight']['#default_value'] = $weight++;
  }
  $form['parameters']['more'] = array(
    '#type' => 'submit',
    '#value' => t('Add more'),
    '#limit_validation_errors' => array(array('parameters')),
    '#submit' => array('wsclient_ui_more_parameter_submit'),
  );
  $form['result_type'] = array(
    '#type' => 'select',
    '#title' => t('Result type'),
    '#options' => array(0 => '--') + $types,
    '#default_value' => isset($operation['result']['type']) ? _wsclient_ui_type_default($types, $service, $operation['result']) : 0,
    '#description' => t('The result data type returned by the service'),
  );
  $form['result_label'] = array(
    '#type' => 'textfield',
    '#title' => t('Result label'),
    '#default_value' => isset($operation['result']['label']) ? $operation['result']['label'] : '',
    '#description' => t('The human-readable name of the result variable returned by the service.'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form_state['service'] = $service;
  $form_state['operation'] = $operation;
  return $form;
}

/**
 * Generates a row in the parameter table.
 */
function _wsclient_ui_parameter_row($service, $types, $name = '', $info = array()) {
  $default_value = _wsclient_ui_type_default($types, $service, $info);
  $parameter['type'] = array(
    '#type' => 'select',
    '#options' => array(0 => '--') + $types,
    '#default_value' => $default_value,
  );
  $parameter['name'] = array(
    '#type' => 'textfield',
    '#size' => 40,
    '#default_value' => $name,
    '#element_validate' => array('wsclient_ui_parameter_name_validate'),
  );
  $parameter['weight'] = array(
    '#type' => 'weight',
  );
  return $parameter;
}

/**
 * Validation callback for parameter names.
 */
function wsclient_ui_parameter_name_validate($element, &$form_state) {
  if ($element['#value'] && !preg_match('!^[A-Za-z0-9_]+$!', $element['#value'])) {
    form_error($element, t('Parameter names must contain only letters, numbers, and underscores.'));
  }
}

/**
 * Submit callback for adding more parameter rows.
 */
function wsclient_ui_more_parameter_submit($form, &$form_state) {
  $form_state['more']++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit callback of operation form.
 */
function wsclient_ui_operation_submit($form, &$form_state) {
  $service = $form_state['service'];
  $operation['label'] = $form_state['values']['label'];
  foreach ($form_state['values']['parameters']['items'] as $key => $item) {
    if (!empty($item['name'])) {
      $operation['parameter'][$item['name']] = array('type' => $item['type']);
    }
  }
  if (!empty($form_state['values']['result_type'])) {
    $operation['result'] = array(
      'type' => $form_state['values']['result_type'],
      'label' => isset($form_state['values']['result_label']) ? $form_state['values']['result_label'] : 'result',
    );
  }
  unset($service->operations[$form_state['operation']['name']]);
  $service->operations[$form_state['values']['name']] = $operation;
  $service->save();
  $form_state['redirect'] = WSCLIENT_UI_PATH . '/manage/' . $service->name;
}

/**
 * Validation callback for operation names.
 */
function wsclient_ui_operation_name_validate($element, &$form_state) {
  if ($element['#value'] && !preg_match('!^[A-Za-z0-9_]+$!', $element['#value'])) {
    form_error($element, t('Operation names must contain only letters, numbers, and underscores.'));
  }
  if ($element['#value'] != $form_state['operation']['name'] && isset($form_state['service']->operations[$element['#value']])) {
    form_error($element, t('An operation with that name already exists'));
  }
}

/**
 * Themes the operation form for editing the used parameters.
 *
 * @ingroup themeable
 */
function theme_wsclient_ui_variable_form($variables) {
  $elements = $variables['element'];

  $table['#theme'] = 'table';
  $table['#header'] = array(t('Data type'), t('Name'), array('data' => t('Weight'), 'class' => array('tabledrag-hide')));
  $table['#attributes']['id'] = 'rules-' . drupal_html_id($elements['#title']) . '-id';

  foreach (element_children($elements['items']) as $key) {
    $element = &$elements['items'][$key];
    // Add special classes to be used for tabledrag.js.
    $element['weight']['#attributes']['class'] = array('rules-element-weight');

    $row = array();
    $row[] = array('data' => $element['type']);
    $row[] = array('data' => $element['name']);
    $row[] = array('data' => $element['weight']);
    $row = array('data' => $row) + $element['#attributes'];
    $row['class'][] = 'draggable';
    $table['#rows'][] = $row;
  }
  $elements['items']['#printed'] = TRUE;
  if (!empty($table['#rows'])) {
    drupal_add_tabledrag($table['#attributes']['id'], 'order', 'sibling', 'rules-element-weight');
  }

  // Theme it like a form item, but with the description above the content.
  $attributes['class'][] = 'form-item';
  $attributes['class'][] = 'rules-variables-form';

  $output = '<div' . drupal_attributes($attributes) . '>' . "\n";
  $output .= theme('form_element_label', $variables);
  if (!empty($elements['#description'])) {
    $output .= ' <div class="description">' . $elements['#description'] . "</div>\n";
  }
  $output .= ' ' . drupal_render($table) . "\n";
  // Add in any further children elements.
  foreach (element_children($elements, TRUE) as $key) {
    $output .= drupal_render($elements[$key]);
  }
  $output .= "</div>\n";
  return $output;
}

/**
 * Returns a list of available data types for service parameters and return
 * values. Entities are excluded.
 */
function wsclient_ui_types() {
  $cache = rules_get_cache();
  $data_info = $cache['data_info'];
  $entity_info = entity_get_info();
  // Remove entities.
  $data_info = array_diff_key($data_info, $entity_info);
  unset($data_info['entity']);
  $options = array();
  foreach ($data_info as $type => $properties) {
    $options[$type] = $properties['label'];
  }
  natcasesort($options);
  return $options;
}

/**
 * FAPI callback to validate the form for editing parameter info.
 */
function wsclient_ui_validate_parameters($elements, &$form_state) {
  $names = array();
  foreach (element_children($elements['items']) as $item_key) {
    $element = &$elements['items'][$item_key];
    if ($element['name']['#value'] || $element['type']['#value']) {
      foreach (array('name' => t('Name'), 'type' => t('Data type')) as $key => $title) {
        if (!$element[$key]['#value']) {
          form_error($element[$key], t('!name field is required.', array('!name' => $title)));
        }
      }
      if (isset($names[$element['name']['#value']])) {
        form_error($element['name'], t('The name %name is already taken.', array('%name' => $element['name']['#value'])));
      }
      $names[$element['name']['#value']] = TRUE;
    }
  }
}

/**
 * Determines and returns a data type from the given service and data info.
 */
function _wsclient_ui_type_default($types, $service, $data_info) {
  if (isset($data_info['type'])) {
    if (isset($types[$data_info['type']])) {
      return $data_info['type'];
    }
    else {
      // Map the internal type name to the global one.
      return 'wsclient_' . $service->name . '_' . $data_info['type'];
    }
  }
  return 0;
}

/**
 * Operation delete confirmation form.
 */
function wsclient_ui_operation_delete($form, &$form_state, $service, $operation) {
  $confirm_question = t('Are you sure you want to delete the operation %operation?', array('%operation' => $operation['label']));
  $form_state['service'] = $service;
  $form_state['operation'] = $operation;
  return confirm_form($form, $confirm_question, WSCLIENT_UI_PATH . '/manage/' . $service->name);
}

/**
 * Submit callback of operation delete form.
 */
function wsclient_ui_operation_delete_submit($form, &$form_state) {
  $service = $form_state['service'];
  $operation = $form_state['operation'];
  unset($service->operations[$operation['name']]);
  $service->save();
  drupal_set_message(t('Deleted operation %operation.', array('%operation' => $operation['label'])));
  $form_state['redirect'] = WSCLIENT_UI_PATH . '/manage/' . $service->name;
}

/**
 * Data type form.
 */
function wsclient_ui_type($form, &$form_state, $service, $type, $op = 'edit') {
  if ($op == 'add') {
    $type = array(
      'name' => '',
      'parameter' => array(),
    );
  }
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => isset($type['label']) ? $type['label'] : '',
    '#required' => TRUE,
    '#description' => t('The human-readable name of the data type.'),
  );
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#default_value' => isset($type['name']) ? $type['name'] : '',
    '#required' => TRUE,
    '#description' => t('The machine-readable name of this data type is used internally to identify the data type.'),
    '#element_validate' => array('wsclient_ui_type_name_validate'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form_state['service'] = $service;
  $form_state['type'] = $type;
  return $form;
}

/**
 * Submit callback of data type form.
 */
function wsclient_ui_type_submit($form, &$form_state) {
  $service = $form_state['service'];
  $type['label'] = $form_state['values']['label'];
  unset($service->datatypes[$form_state['type']['name']]);
  $service->datatypes[$form_state['values']['name']] = $type;
  $service->save();
  $form_state['redirect'] = WSCLIENT_UI_PATH . '/manage/' . $service->name;
}

/**
 * Validation callback for data type names.
 */
function wsclient_ui_type_name_validate($element, &$form_state) {
  if ($element['#value'] && !preg_match('!^[A-Za-z0-9_]+$!', $element['#value'])) {
    form_error($element, t('Data type names must contain only letters, numbers, and underscores.'));
  }
  if ($element['#value'] != $form_state['type']['name'] && isset($form_state['service']->datatypes) && in_array($element['#value'], array_keys($form_state['service']->datatypes))) {
    form_error($element, t('A data type with that name already exists'));
  }
}

/**
 * Helper function to sort a nested data information array based on the label
 * of the items.
 */
function wsclient_ui_label_sort($data_info, $label_key = 'label') {
  $sort_info = array();
  foreach ($data_info as $key => $info) {
    $sort_info[$key] = $info[$label_key];
  }
  natcasesort($sort_info);
  foreach ($sort_info as $key => $label) {
    $sort_info[$key] = $data_info[$key];
  }
  return $sort_info;
}
