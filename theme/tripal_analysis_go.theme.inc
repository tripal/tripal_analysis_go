<?php

/*******************************************************************************
 *
*/
function tripal_analysis_go_preprocess_tripal_feature_go_results(&$variables) {
  $feature = $variables['node']->feature;
  $db_id = null;

  $go_terms = tripal_get_feature_go_results($feature->feature_id);
  $feature->tripal_analysis_go = new stdClass;
  $feature->tripal_analysis_go->go_terms = $go_terms;
}

/**
 *
 * @ingroup tripal_analysis_go
 */
function tripal_analysis_go_preprocess_tripal_organism_go_summary(&$variables) {
  $node = $variables['node'];
  $organism = $node->organism;
  if (!property_exists($organism, 'tripal_analysis_go') || !is_object($organism->tripal_analysis_go)){
    $organism->tripal_analysis_go = new stdClass;
  }
  $organism->tripal_analysis_go->select_form = tripal_analysis_go_load_organism_go_summary($node);
}

/**
 *
 * @param $node
 *
 */
function tripal_analysis_go_load_organism_go_summary($node) {
  $organism = $node->organism;

  // find analyses that have go terms
  $sql = "
    SELECT analysis_id
    FROM {go_count_analysis} GO
    WHERE organism_id = :organism_id
    ORDER BY analysis_id DESC
  ";

  $results = chado_query($sql, array(':organism_id' => $organism->organism_id));

  # check to ensure we have access to at least one of the GO analyses
  $has_results = 0;
  while ($analysis = $results->fetchObject()) {
    $nid = chado_get_nid_from_id('analysis', $analysis->analysis_id);
    $anode = node_load($nid);
    if (node_access("view", $anode)) {
      $has_results = 1;
    }
  }
  return array(
    'has_results' => $has_results,
    'form' => drupal_get_form('tripal_analysis_go_select_form', $node),
  );
}

/**
*
* @param $form
* @param $form_state
* @param $node
*
*/
function tripal_analysis_go_select_form($form, $form_state, $node) {

  $form = array();

  // find analyses that have GO terms and their organisms.  this is kept
  // in the go_count_analysis materialized view
  $sql = "
    SELECT *
    FROM {go_count_analysis} GO, {analysis} a
    WHERE organism_id = :organism_id
    AND GO.analysis_id = a.analysis_id
    ORDER BY GO.analysis_id DESC
  ";
  $results = chado_query($sql, array(':organism_id' => $node->organism->organism_id));

  // now build the list of analyses to use in the drop down element
  $analyses = array();
  $analyses[''] = '';
  while ($analysis = $results->fetchObject()) {
    # check to see if the user has permission to see this analysis
    $nid = chado_get_nid_from_id('analysis', $analysis->analysis_id);
    $anode = node_load($nid);
    if (node_access("view", $anode)) {
      $analyses[$analysis->analysis_id."-".$node->organism->organism_id] = "$analysis->name";
    }
  }

  // now generate the select box
  $form['tripal_analysis_go_select'] = array(
    '#title' => t('Select a GO report to view'),
    '#description' => t('Any analysis with GO results related to this organism are available for viewing. For further information, see the analysis information page.'),
    '#type'  => 'select',
    '#options' => $analyses,
    '#ajax' => array(
      'callback' => "tripal_analysis_go_org_report",
      'wrapper' => 'tripal-analyis-go-report',
      'effect'   => 'fade',
      'method'   => 'replace',
    ),
    '#suffix' => '<div id="tripal-analyis-go-report"></div>'
  );
  return $form;
}

/**
 *
 * @param $analysis_id
 */
function tripal_analysis_go_org_report($form, $form_state) {

    $analysis_id = $form_state['values']['tripal_analysis_go_select'];
    $content = tripal_analysis_go_full_report($analysis_id);

    $commands = array();

    $commands[] = ajax_command_html('#tripal-analyis-go-report', $content);

    $commands[] = ajax_command_invoke(NULL, 'tripal_analysis_go_init_tree');

    $commands[] = ajax_command_invoke(NULL, 'tripal_analysis_go_init_graph');

    return  array('#type' => 'ajax', '#commands' => $commands);

  $analysis_id = $form_state['values']['tripal_analysis_go_select'];
  $content = tripal_analysis_go_full_report($analysis_id);
  return '<div id="tripal-analyis-go-report">' . $content . '</div>';
}
/**
 *
 * @param  $analysis_id
 *
 */
function tripal_analysis_go_full_report($ids) {
   $analysis_id = preg_replace("/^(\d+)-(\d+)$/","$1",$ids);
   $organism_id = preg_replace("/^(\d+)-(\d+)$/","$2",$ids);
   $content = '';

   if($analysis_id and $organism_id){
      $content = "

        <b>Biological Process</b>
        <br><i>Expand the tree to browse term counts. Click a term to view term details.</i>
        <div class=\"tripal_cv_tree\"  id=\"tripal_analysis_go_cv_tree_".$organism_id."-".$analysis_id."_bp\"></div>
	      <br><br><div class=\"tripal_cv_chart\" id=\"tripal_analysis_go_cv_chart_".$organism_id."-".$analysis_id."_bp\"></div>
	      <br><br><br><br>

        <b>Cellular Component</b>
        <br><i>Expand the tree to browse term counts. Click a term to view term details.</i>
        <div class=\"tripal_cv_tree\"  id=\"tripal_analysis_go_cv_tree_".$organism_id."-".$analysis_id."_cc\"></div>
        <br><br><div class=\"tripal_cv_chart\" id=\"tripal_analysis_go_cv_chart_".$organism_id."-".$analysis_id."_cc\"></div>

        <br><br><br><br>
        <b>Molecular Function</b>
        <br><i>Expand the tree to browse term counts. Click a term to view term details.</i>
        <div class=\"tripal_cv_tree\"  id=\"tripal_analysis_go_cv_tree_".$organism_id."-".$analysis_id."_mf\"></div>
        <br><br><div class=\"tripal_cv_chart\" id=\"tripal_analysis_go_cv_chart_".$organism_id."-".$analysis_id."_mf\"></div>
      ";
   }

   return $content;
}

/*******************************************************************************
 */
function tripal_analysis_go($analysis_id, $type_id = 0, $ajax, $is_v0_3=0) {

  drupal_add_library('system', 'drupal.ajax');

  $headers = array(
    'GO Name',
    'GO Terms'
  );

  $rows = array();

  // List all terms on the left. The SQL statement below can use
  // the older method for CVTerms with BRITE hierarchies or the newer method
  $sql = "
    SELECT DISTINCT CVT.name, CVT.cvterm_id
    FROM {cvterm} CVT
      INNER JOIN {analysisprop} AP ON CVT.cvterm_id = AP.type_id
      INNER JOIN {cv} CV on CV.cv_id = CVT.cv_id
    WHERE AP.analysis_id = :analysis_id AND
      (CVT.definition LIKE 'GO term: %' or CV.name = 'GO')
    ORDER BY CVT.cvterm_id";

  $result = chado_query($sql, array(':analysis_id' => $analysis_id));

  $brite_list = '<ul>';
  while ($brite_term = $result->fetchObject()) {
    $link = l($brite_term->name, "tripal_analysis_go/report/nojs/$analysis_id/$brite_term->cvterm_id",
        array('attributes' => array('class' => 'use-ajax', 'onclick' => 'javascript:void(0);')));
    $brite_list .= '<li>' . $link . '</li>';
  }
  $brite_list .= '</ul>';

  // If called by ajax, generate tree structure
  $rows[] = array(
    array(
      'data' => $brite_list,
      'valign' => 'top',
      'width' => '25%',
    ),
    array(
      'data' => '<div id="heirarchy_terms_list">' . t('<i>Note:</i> Click a BRITE heirarchy name to browse term assignments.') . '<div>',
      'valign' => 'top',
    ),
  );

  // the $table array contains the headers and rows array as well as other
  // options for controlling the display of the table.  Additional
  // documentation can be found here:
  // https://api.drupal.org/api/drupal/includes%21theme.inc/function/theme_table/7
  $table = array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(
      'id' => 'tripal_analysis_go-table-report',
    ),
    'sticky' => FALSE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => '',
  );

  $content = theme_table($table);
  $content .= "<div id='ajax-response-goes-here'></div>";
  return $content;

}
/**
 *
 */
function tripal_analysis_go_ajax_update($analysis_id, $cvterm_id) {
 // Get BRITE term from cvterm table

  $sql = 'SELECT name FROM {cvterm} WHERE cvterm_id = :cvterm_id';
  $brite_term = chado_query($sql, array(':cvterm_id' => $cvterm_id))->fetchField();

  // Get BRITE hierarchy tree for this analysis
  $sql = "
    SELECT value
    FROM {analysisprop} AP
      INNER JOIN {CVterm} CVT on AP.type_id = CVT.cvterm_id
      INNER JOIN {CV} on CVT.cv_id = CV.cv_id
    WHERE (CV.name = 'tripal' or CV.name = 'GO') and CVT.name = :name
     AND AP.analysis_id = :analysis_id
  ";
  $result = chado_query($sql, array(':name' => $brite_term, 'analysis_id' => $analysis_id))->fetchObject();

  $content = "<div id=\"heirarchy_terms_list\"><b>$brite_term</b>" . $result->value . '</div>';

  $commands = array();
  $commands[] = ajax_command_replace('#heirarchy_terms_list', $content);
  return array(
    '#type' => 'ajax',
    '#commands' => $commands,
  );
}
