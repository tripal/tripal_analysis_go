<?php

/**
 * Launchpad for GO administration.
 *
 * @ingroup tripal_analysis_go
 */
function tripal_analysis_go_admin_view() {
  $output = '';

  // set the breadcrumb
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l('Administration', 'admin');
  $breadcrumb[] = l('Tripal', 'admin/tripal');
  $breadcrumb[] = l('Extensions', 'admin/tripal/extension');
  $breadcrumb[] = l('GO Analysis', 'admin/tripal/extension/tripal_analysis_go');
  drupal_set_breadcrumb($breadcrumb);

  $output .= "The Tripal GO Module extends the Tripal Analysis module to
    provide support loading and display of GO analyses for features that
    reside in the database.";

  return $output;
}

/**
 * A form for setting administrative options
 */
function tripal_analysis_go_settings_form($form, &$form_state) {
  $form = array();

  // Add fields for setting the node titles
  $details = array(
    'module' => 'tripal_analysis_go',       // the name of the MODULE implementing the content type
    'content_type' => 'chado_analysis_go',   // the name of the content type
    'options' => array(
      '[analysis.name]' => 'Analysis Name Only',
      '[analysis.name] ([analysis.sourcename]) [analysis.program] version [analysis.programversion]' => 'Unique Constraint: Includes the name, source and program name/version'
    ),
    'unique_option' => '[analysis.name] ([analysis.sourcename]) [analysis.program] version [analysis.programversion]'
  );
  chado_add_admin_form_set_title($form, $form_state, $details);

  // Add form elements for customizing the node URLs.
  $details = array(
    'module' => 'tripal_analysis_go',
    'content_type' => 'chado_analysis_go',
    // An array of options to use under "Page URL"
    // the key should be the token and the value should be the human-readable option
    'options' => array(
      '/analysis/[analysis.analysis_id]' => 'Analysis ID',
      '/analysis/[analysis.program]/[analysis.programversion]/[analysis.sourcename]' => 'Unique Contraint: Includes the program name & version as well as the source name'
    )
  );
  chado_add_admin_form_set_url($form, $form_state, $details);

  return $form;
}

/**
 * Implements hook_chado_node_default_url_format()
 */
function chado_analysis_go_chado_node_default_url_format() {
  return '/analysis/[analysis.analysis_id]';
}
