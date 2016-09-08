<?php

/**
 * Implements hook_node_info();
 */
function tripal_analysis_go_node_info() {
  $nodes = array();
  $nodes['chado_analysis_go'] = array(
    'name' => t('Analysis: GO'),
    'base' => 'chado_analysis_go',
    'description' => t('A GO analysis from the chado database'),
    'has_title' => FALSE,
    'title_label' => t('Analysis: GO'),
    'has_body' => FALSE,
    'body_label' => t('GO Analysis Description'),
    'locked' => TRUE,
    'chado_node_api' => array(
      'base_table' => 'analysis',
      'hook_prefix' => 'chado_analysis_go',
      'linking_table' => 'chado_analysis',
      'record_type_title' => array(
        'singular' => t('GO Analysis'),
        'plural' => t('GO Analyses')
      ),
      'sync_filters' => array(
        'type_id' => FALSE,
        'organism_id' => FALSE,
        'checkboxes' => array('name'),
      ),
    )
  );
  return $nodes;
}

/**
 * Implements hook_chado_node_sync_select_query().
 */
function chado_analysis_go_chado_node_sync_select_query($query) {
  $query['joins'][] = 'INNER JOIN {analysisprop} PROP ON PROP.analysis_id = ANALYSIS.analysis_id';
  $query['joins'][] = 'INNER JOIN {cvterm} CVTP ON CVTP.cvterm_id = PROP.type_id';
  $query['where_clauses']['analysis_type'][] = 'CVTP.name = :type_name';
  $query['where_clauses']['property_value'][] = 'PROP.value = :prop_value';
  $query['where_args']['analysis_type'][':type_name'] = 'Analysis Type';
  $query['where_args']['property_value'][':prop_value'] = 'go_analysis';

  return $query;
}

/*******************************************************************************
 *  The following function proves access control for users trying to
*  perform actions on data managed by this module
*/
function chado_analysis_go_access($op,  $node, $account) {
  $node_type = $node;
  if (is_object($node)) {
    $node_type = $node->type;
  }

  if($node_type == 'chado_analysis_go') {
    if ($op == 'create') {
      if (!user_access('create chado_analysis_go content',   $account)) {
        return NODE_ACCESS_DENY;
      }
      return NODE_ACCESS_ALLOW;
    }
    if ($op == 'update') {
      if (!user_access('edit chado_analysis_go content',   $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    if ($op == 'delete') {
      if (!user_access('delete chado_analysis_go content',   $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    if ($op == 'view') {
      if (!user_access('access chado_analysis_go content',   $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    return NODE_ACCESS_IGNORE;
  }
}

/*******************************************************************************
 *  Provide a GO Analysis form
 */
function chado_analysis_go_form($node, $form_state) {

  // add in the default fields for the analysis
  $form = chado_analysis_form($node, $form_state);

  // set the defaults
  $gaf_file = '';
  $gaf_file_ext = '';
  $seq_type = '';
  $query_uniquename = '';
  $method = 1;
  $re_name = '';
  $gojob = FALSE;

  if (property_exists ( $node, 'analysis' )) {
    $analysis = $node->analysis->tripal_analysis_go;
    $gaf_file = $analysis->gaf_file;
    $gaf_file_ext = $analysis->gaf_file_ext;
    $seq_type = $analysis->seq_type;
    $query_uniquename = $analysis->query_uniquename;
    $method = $analysis->method;
    $re_name = $analysis->re_name;
    $gojob = $analysis->gojob;
  }
  // if we are re constructing the form from a failed validation or ajax callback
  // then use the $form_state['values'] values
  if (array_key_exists('values', $form_state)) {
    $gaf_file           = $form_state['values']['gaf_file'];
    $gaf_file_ext       = $form_state['values']['gaf_file_ext'];
    $seq_type           = $form_state['values']['seq_type'];
    $query_uniquename   = $form_state['values']['query_uniquename'];
    $method             = $form_state['values']['method'];
    $re_name            = $form_state['values']['re_name'];
    $gojob              = $form_state['values']['gojob'];
  }
  // if we are re building the form from after submission (from ajax call) then
  // the values are in the $form_state['input'] array
  if (array_key_exists('input', $form_state) and !empty($form_state['input'])) {
    $gaf_file           = $form_state['input']['gaf_file'];
    $gaf_file_ext       = $form_state['input']['gaf_file_ext'];
    $seq_type           = $form_state['input']['seq_type'];
    $query_uniquename   = $form_state['input']['query_uniquename'];
    $method             = $form_state['input']['method'];
    $re_name            = $form_state['input']['re_name'];
    $gojob              = $form_state['input']['gojob'];
  }

  $form['go'] = array(
    '#title' => t('GO Settings'),
    '#type' => 'fieldset',
    '#description' => t('Specific Settings for GO Analysis.'),
    '#collapsible' => TRUE,
    '#attributes' => array('id' => 'go-extra-settings'),
    '#group' => 'chado_node_api',
  );

   $form['go']['notice']= array(
      '#markup' => t('<b>Note:</b> currently, the GAF loader only uses column 2 (Object ID) and 5 (GO ID)
          from the GAF file, and simply imports GO terms for the features.'),
   );

   $form['go']['gaf_file']= array(
      '#type'          => 'textfield',
      '#title'         => t('GAF File'),
      '#description'   => t('Please enter the full system path for the GAF file, or a path within the Drupal
                             installation (e.g. /sites/default/files/xyz.txt).  The path must be accessible to the
                             server on which this Drupal instance is running.'),
      '#required' => TRUE,
      '#default_value' => $gaf_file,
  );

   $form['go']['gaf_file_ext']= array(
      '#type'          => 'textfield',
      '#title'         => t('GAF File extension'),
      '#description'   => t('If a directory is provide for the GAF file setting above,
                             then a file extension can be provided here. Files with this extension
                             in the directory will be parsed. If no extension is provided then files
                             with a .gaf extension will be parsed within the directory.
                             Please provide the extension without the preceeding period (e.g. "out" rather than ".out"'),
      '#required' => FALSE,
      '#default_value' => $gaf_file_ext,
  );

  $cv = tripal_get_cv(array('name' => 'sequence'));
  $cv_id = $cv->cv_id;
  $form['go']['seq_type']= array(
      '#type' => 'textfield',
      '#title' => t('Sequence Type'),
      '#required' => TRUE,
      '#description' => t('Please enter the Sequence Ontology term that describes the sequences in the GAF file.'),
      '#default_value' => $seq_type,
      '#autocomplete_path' => "admin/tripal/chado/tripal_cv/cvterm/auto_name/$cv_id",
  );
  $form['go']['query_uniquename'] = array(
      '#title' => t('Use Unique Name'),
      '#type' => 'checkbox',
      '#description' => t('Select this checboxk if the feature name in the GAF file '.
        'matches the uniquename in the database.  By default, the feature will '.
        'be mapped to the "name" of the feature.'),
      '#default_value' => $query_uniquename,
	);
   $form['go']['import_options'] = array(
      '#type' => 'fieldset',
      '#title' => t('Import Options'),
      '#collapsed' => TRUE
   );

   $form['go']['import_options']['method']= array(
      '#type' => 'radios',
      '#title' => t('Import options'),
      '#required' => TRUE,
      '#options' => array(t('Add GO terms')/*,t('Replace GO terms')*/,t('Remove GO terms')),
      '#description' => t('Select if GO terms in the GAF file will be added to each feature or removed.'),
      '#default_value' => $method,
   );

  $form['go']['import_options']['gojob'] = array(
      '#type' => 'checkbox',
      '#title' => t('Submit a job to load the GAF content into Chado'),
      '#description' => t('Note: features associated with the GO results must '.
                          'exist in chado before parsing the file. Otherwise, GO '.
                          'results that cannot be linked to a feature will be '.
                          'discarded. '),
      '#default_value' => $gojob
  );

  // Advanced Options
   $form['go']['advanced'] = array(
      '#type' => 'fieldset',
      '#title' => t('Advanced Options'),
      '#collapsed' => TRUE
   );
   $form['go']['advanced']['re_help']= array(
      '#type' => 'item',
      '#value' => t('A regular expression is an advanced method for extracting information from a string of text.
                     By default, this loader will use the first word in the second column of the GAF file
                     as the uniquename for the sequences.  If this is not desired, you may use the following regular
                     expressions to define the location of the name or unique name within the text of column 2.'),
   );

   $form['go']['advanced']['re_name']= array(
      '#type' => 'textfield',
      '#title' => t('Regular expression for the name'),
      '#required' => FALSE,
      '#description' => t('Enter the regular expression that will extract the '.
         'feature name from the GAF file. This option is '.
         'is only required when the feature identifier does not identically match a feature '.
         'in the database.'),
         '#default_value' => $re_name,
   );

   return $form;
}
/**
*
*/
function chado_analysis_go_validate($node,  &$form, &$form_state) {

  // use the analysis parent to validate the node
  tripal_analysis_validate($node,  $form, $form_state);

  // trim character fields
  $node->gaf_file         = trim($node->gaf_file);
  $node->gaf_file_ext         = trim($node->gaf_file_ext);
  $node->seq_type        = trim($node->seq_type);
  $node->query_uniquename  = trim($node->query_uniquename);
  $node->re_uniquename  = trim($node->query_uniquename);

  if($node->gaf_file and !is_readable($node->gaf_file)) {
    form_set_error('gaf_file', "Can not read GAF file: '$node->gaf_file'. Please check file permissions or typos in the file path.");
  }
}

function chado_analysis_go_load($nodes) {

  // load the default set of analysis fields
  chado_analysis_load($nodes);

  foreach ($nodes as $nid => $node) {

    // Get the analysis.  If it doesn't exist this may be an orphaned node.
    $analysis = $node->analysis;
    if (!$analysis) {
      continue;
    }
    $analysis_id = $analysis->analysis_id;
    $record = array('table'=> 'analysis', 'id' => $analysis->analysis_id);

    // Get the go settings.
    $gaf_file = chado_get_property($record, array('type_name' => 'analysis_go_gaf_file', 'cv_name' => 'tripal'));
    $gaf_file_ext = chado_get_property($record, array('type_name' => 'analysis_go_gaf_file_ext', 'cv_name' => 'tripal'));
    $seq_type = chado_get_property($record, array('type_name' => 'analysis_go_seq_type', 'cv_name' => 'tripal'));
    $query_uniquename = chado_get_property($record, array('type_name' => 'analysis_go_query_uniquename', 'cv_name' => 'tripal'));
    $method = chado_get_property($record, array('type_name' => 'analysis_go_method', 'cv_name' => 'tripal'));
    $re_name = chado_get_property($record, array('type_name' => 'analysis_go_re_name', 'cv_name' => 'tripal'));
    $gojob = chado_get_property($record, array('type_name' => 'analysis_go_gojob', 'cv_name' => 'tripal'));

    $analysis->tripal_analysis_go = new stdClass();
    $analysis->tripal_analysis_go->gaf_file         = is_object($gaf_file)        ? $gaf_file->value        : '';
    $analysis->tripal_analysis_go->gaf_file_ext     = is_object($gaf_file_ext)        ? $gaf_file_ext->value        : '';
    $analysis->tripal_analysis_go->seq_type         = is_object($seq_type)       ? $seq_type->value       : '';
    $analysis->tripal_analysis_go->query_uniquename = is_object($query_uniquename) ? $query_uniquename->value : '';
    $analysis->tripal_analysis_go->method           = is_object($method)  ? $method->value  : '';
    $analysis->tripal_analysis_go->re_name          = is_object($re_name)  ? $re_name->value  : '';
    $analysis->tripal_analysis_go->gojob            = is_object($gojob)  ? $gojob->value  : '';

    // Now get the title
    $node->title = chado_get_node_title($node);

    $nodes[$nid]->analysis = $analysis;
}
}
/**
 *
 */
function chado_analysis_go_insert($node) {

  // insert the analysis using the hook_insert() from the Tripal analysis module
  chado_analysis_insert($node);

  // trim character fields
  $node->gaf_file          = trim($node->gaf_file);
  $node->gaf_file_ext      = trim($node->gaf_file_ext);
  $node->seq_type          = trim($node->seq_type);
  $node->query_uniquename  = trim($node->query_uniquename);
  $node->method            = trim($node->method);
  $node->re_name           = trim($node->re_name);
  $node->gojob             = trim($node->gojob);

  $record = array('table'=> 'analysis', 'id' => $node->analysis_id);

  // Add the analysis type.
  chado_insert_property($record, array('type_name' =>'Analysis Type', 'cv_name' => 'analysis_property', 'value' => 'go_analysis'));

  // Now add in the remaining settings as a single property but separated by bars
  chado_insert_property($record, array('type_name' =>'analysis_go_gaf_file', 'cv_name' => 'tripal', 'value' => trim($node->gaf_file)));
  chado_insert_property($record, array('type_name' =>'analysis_go_gaf_file_ext', 'cv_name' => 'tripal', 'value' => trim($node->gaf_file_ext)));
  chado_insert_property($record, array('type_name' =>'analysis_go_seq_type', 'cv_name' => 'tripal', 'value' => trim($node->seq_type)));
  chado_insert_property($record, array('type_name' =>'analysis_go_query_uniquename', 'cv_name' => 'tripal', 'value' => trim($node->query_uniquename)));
  chado_insert_property($record, array('type_name' =>'analysis_go_method', 'cv_name' => 'tripal', 'value' => trim($node->method)));
  chado_insert_property($record, array('type_name' =>'analysis_go_re_name', 'cv_name' => 'tripal', 'value' => trim($node->re_name)));
  chado_insert_property($record, array('type_name' =>'analysis_go_gojob', 'cv_name' => 'tripal', 'value' => trim($node->gojob)));

  // Submit the parsing jobs
  chado_analysis_go_submit_job($node);
}

/**
  *
  */
function chado_analysis_go_update($node) {

  // update the anlaysis
  chado_analysis_update($node);

  // trim character fields
  $node->gaf_file           = trim($node->gaf_file);
  $node->gaf_file_ext       = trim($node->gaf_file_ext);
  $node->seq_type           = trim($node->seq_type);
  $node->query_uniquename   = trim($node->query_uniquename);
  $node->method             = trim($node->method);
  $node->re_name            = trim($node->re_name);
  $node->gojob              = trim($node->gojob);

  $record = array('table'=> 'analysis', 'id' => $node->analysis_id);
  $options = array('insert_if_missing' => TRUE);

  // Update the analysis type.
  chado_update_property($record, array('type_name' =>'Analysis Type', 'cv_name' => 'analysis_property', 'value' => 'go_analysis'), $options);

  // Update the go settings.
  chado_update_property($record, array('type_name' =>'analysis_go_gaf_file', 'cv_name' => 'tripal', 'value' => trim($node->gaf_file)), $options);
  chado_update_property($record, array('type_name' =>'analysis_go_gaf_file_ext', 'cv_name' => 'tripal', 'value' => trim($node->gaf_file_ext)), $options);
  chado_update_property($record, array('type_name' =>'analysis_go_seq_type', 'cv_name' => 'tripal', 'value' => trim($node->seq_type)), $options);
  chado_update_property($record, array('type_name' =>'analysis_go_query_uniquename', 'cv_name' => 'tripal', 'value' => trim($node->query_uniquename)), $options);
  chado_update_property($record, array('type_name' =>'analysis_go_method', 'cv_name' => 'tripal', 'value' => trim($node->method)), $options);
  chado_update_property($record, array('type_name' =>'analysis_go_re_name', 'cv_name' => 'tripal', 'value' => trim($node->re_name)), $options);
  chado_update_property($record, array('type_name' =>'analysis_go_gojob', 'cv_name' => 'tripal', 'value' => trim($node->gojob)), $options);

  // submit the parsing jobs
  chado_analysis_go_submit_job($node);
}

/**
 * Delete go anlysis
 */
function chado_analysis_go_delete($node) {
  chado_analysis_delete($node);
}

/**
 *
 */
function chado_analysis_go_submit_job($node) {
  global $user;

  // Add a job if the user wants to parse the xml output
  if ($node->gojob) {

    $job_args = array(
        $node->analysis_id,
        $node->gaf_file,
        $node->gaf_file_ext,
        $node->seq_type,
        $node->query_uniquename,
        $node->method,
        $node->re_name,
        $node->gojob,
    );

    tripal_add_job("Load GAF 2.0 file: $node->gaf_file", 'tripal_analysis_go', 'tripal_analysis_go_load_gaf', $job_args, $user->uid);
  }
}

/**
 * Implements hook_node_view().
 */
function tripal_analysis_go_node_view($node, $view_mode, $langcode) {

  switch ($node->type) {
    case 'chado_analysis_go':
      if ($view_mode == 'full') {
        $node->content['tripal_analysis_go_base'] = array(
          '#markup' => theme('tripal_analysis_go_base', array('node' => $node)),
          '#tripal_toc_id'    => 'base',
          '#tripal_toc_title' => 'Overview',
          '#weight' => -100,
        );
      }
      if ($view_mode == 'teaser') {
        $node->content['tripal_analysis_go_teaser'] = array(
          '#markup' => theme('tripal_analysis_go_teaser', array('node' => $node)),
        );
      }
      break;
    case 'chado_organism':
      if ($view_mode == 'full') {
        $node->content['tripal_organism_go_summary'] = array(
          '#markup' => theme('tripal_organism_go_summary', array('node' => $node)),
          '#tripal_toc_id'    => 'GO',
          '#tripal_toc_title' => 'GO Analysis Reports',
        );
      }
      break;
    case 'chado_feature':
      if ($view_mode == 'full') {
        $node->content['tripal_feature_go_results'] = array(
          '#markup' => theme('tripal_feature_go_results', array('node' => $node)),
          '#tripal_toc_id'    => 'go',
          '#tripal_toc_title' => 'GO annotation',
        );
      }
      break;
   }
   return $node;
 }

/**
 * Implements [content_type]_chado_node_default_title_format().
 *
 * Defines a default title format for the Chado Node API to set the titles on
 * Chado Analysis nodes based on chado fields.
 */
function chado_analysis_go_chado_node_default_title_format() {
  return '[analysis.name]';
}

/**
 * Implements hook_node_insert().
 * Acts on all content types.
 *
 * @ingroup tripal_analysis_go
 */
function tripal_analysis_go_node_insert($node) {

  switch ($node->type) {
    case 'chado_analysis_go':

      // build the analysis variable
      $analysis_id = chado_get_id_from_nid('analysis', $node->nid);
      $values = array('analysis_id' => $analysis_id);
      $analysis = chado_generate_var('analysis', $values);
      $node->analysis = $analysis;

      // Now get the title
      $node->title = chado_get_node_title($node);

      // Now use the API to set the path.
      chado_set_node_url($node);

      break;
  }
}

/**
 * Implements hook_node_update().
 * Acts on all content types.
 *
 * @ingroup tripal_analysis_go
 */
function tripal_analysis_go_node_update($node) {

  switch ($node->type) {
  case 'chado_analysis_go':

    // build the analysis variable
    $analysis_id = chado_get_id_from_nid('analysis', $node->nid);
    $values = array('analysis_id' => $analysis_id);
    $analysis = chado_generate_var('analysis', $values);
    $node->analysis = $analysis;

    // Now get the title
    $node->title = chado_get_node_title($node);

    break;
  }
}
