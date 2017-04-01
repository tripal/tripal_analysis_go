<?php

function tripal_analysis_go_cv_chart($chart_id){

  // The CV module will create the JSON array necessary for buillding a
  // pie chart using jgChart and Google Charts.  We have to pass to it
  // a table that contains count information, tell it which column
  // contains the cvterm_id and provide a filter for getting the
  // results we want from the table.
  $organism_id = preg_replace("/^tripal_analysis_go_cv_chart_(\d+)-(\d+)_(bp|cc|mf)$/","$1",$chart_id);
  $analysis_id = preg_replace("/^tripal_analysis_go_cv_chart_(\d+)-(\d+)_(bp|cc|mf)$/","$2",$chart_id);
  $type        = preg_replace("/^tripal_analysis_go_cv_chart_(\d+)-(\d+)_(bp|cc|mf)$/","$3",$chart_id);

  $sql = "SELECT * FROM {Analysis} WHERE analysis_id = :analysis_id";
  $analysis = chado_query($sql, array(':analysis_id' => $analysis_id))->fetchObject();


  if(strcmp($type,'mf')==0){
     $class = 'molecular_function';
     $title = "Number of Molecular Function Terms From $analysis->name Analysis";
  }
  if(strcmp($type,'cc')==0){
     $class = 'cellular_component';
     $title = "Number of Cellular Component Terms From $analysis->name Analysis";
  }
  if(strcmp($type,'bp')==0){
     $class = 'biological_process';
     $title = "Number of Biological Process Terms From $analysis->name Analysis";
  }
  $options = array(
     'count_mview'      => 'go_count_analysis',
     'cvterm_id_column' => 'cvterm_id',
     'count_column'     => 'feature_count',
     'filter'           => "
        CNT.organism_id = $organism_id AND
        CNT.analysis_id = $analysis_id AND
        CNT.cvterm_id IN (
          SELECT CVTR.subject_id
          FROM {CVTerm_relationship} CVTR
            INNER JOIN {CVTerm} CVT on CVTR.object_id = CVT.cvterm_id
            INNER JOIN {CV} on CVT.cv_id = CV.cv_id
          WHERE CVT.name = '$class' AND
                 CV.name = '$class'
        )
     ",
     'type'             => 'p',
     'size'             => '650x260',
     'title'            => $title,
  );
  return $options;
}


function tripal_cv_chart($chart_id) {
  // parse out the tripal module name from the chart_id to find out
  // which Tripal "hook" to call:
  $tripal_mod = preg_replace("/^(tripal_.+?)_cv_chart_(.+)$/", "$1", $chart_id);
  $callback = $tripal_mod . "_cv_chart";

  // now call the function in the module responsible for the chart to fill out
  // an options array needed by the tripal_cv_count_chart call below.
  $opt = call_user_func_array($callback, array($chart_id));

  // build the JSON array to return to the javascript caller
  $json_arr = tripal_cv_count_chart(
    $opt['count_mview'],
    $opt['cvterm_id_column'],
    $opt['count_column'],
    $opt['filter'],
    $opt['title'],
    $opt['type'],
    $opt['size']
  );
  $json_arr[] = $chart_id;  // add the chart_id back into the json array

  return drupal_json_output($json_arr);

}

function tripal_cv_count_chart($cnt_table, $fk_column,
  $cnt_column, $filter = NULL, $title = '', $type = 'p3', $size='300x75') {

  if (!$type) {
    $type = 'p3';
  }

  if (!$size) {
    $size = '300x75';
  }

  if (!$filter) {
    $filter = '(1=1)';
  }

  $is_pie = 0;
  if (strcmp($type, 'p') == 0 or strcmp($type, 'p3') == 0) {
    $is_pie = 1;
  }
  $sql = "
    SELECT CVT.name, CVT.cvterm_id, CNT.$cnt_column as num_items
    FROM {" . $cnt_table . "} CNT
     INNER JOIN {cvterm} CVT on CNT.$fk_column = CVT.cvterm_id
    WHERE $filter
  ";

  $features = array();
  $results = chado_query($sql);
  $data = array();
  $axis = array();
  $legend = array();
  $total = 0;
  $i = 1;
  while ($term = $results->fetchObject()) {

    $term_num = intval($term->num_items);
    if ($is_pie) {
      $axis[] = "$term->name";
      $data[] = array($term_num, 0, 0);
    }
    else {
      $axis[] = "$term->name";
      $data[] = array($term_num);
    }
    $total += $term_num;
    $i++;
  }

  $opt[] = array(
    'data' => $data,
    'axis_labels' => $axis,
    'legend' => $legend,
    'size' => $size,
    'type' => $type,

    'bar_width'     => 10,
    'bar_spacing'   => 0,
    'title'         => $title
  );

  return $opt;
}
