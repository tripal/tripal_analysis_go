<?php

function tripal_analysis_go_cv_tree($tree_id){

  $organism_id = preg_replace("/^tripal_analysis_go_cv_tree_(\d+)-(\d+)_(bp|cc|mf)$/", "$1", $tree_id);
  $analysis_id = preg_replace("/^tripal_analysis_go_cv_tree_(\d+)-(\d+)_(bp|cc|mf)$/", "$2", $tree_id);
  $type        = preg_replace("/^tripal_analysis_go_cv_tree_(\d+)-(\d+)_(bp|cc|mf)$/", "$3", $tree_id);

  if(strcmp($type,'mf')==0){
     $class = 'molecular_function';
  }
  if(strcmp($type,'cc')==0){
     $class = 'cellular_component';
  }
  if(strcmp($type,'bp')==0){
     $class = 'biological_process';
  }

  $cv = tripal_get_cv(array('name' => $class));

  $options = array(
     'cv_id'            => $cv->cv_id,
     'count_mview'      => 'go_count_analysis',
     'cvterm_id_column' => 'cvterm_id',
     'count_column'     => 'feature_count',
     'filter'           => "CNT.organism_id = $organism_id AND CNT.analysis_id = $analysis_id",
     'label'            => 'Features',
  );
  return $options;
}

function tripal_cv_tree($tree_id) {
  // parse out the tripal module name from the chart_id to find out
  // which Tripal "hook" to call:
  $tripal_mod = preg_replace("/^(tripal_.+?)_cv_tree_(.+)$/", "$1", $tree_id);
  if ($tripal_mod) {
    $callback = $tripal_mod . "_cv_tree";

    // now call the function in the module responsible for the tree.  This
    // should call the tripal_cv_init_cv with the proper parameters set for
    // getting the cv_id of the vocabulary to use
    $opt = call_user_func_array($callback, array($tree_id));

    // we only need to return the cv_id for this function call.
    $json_array[] = $opt['cv_id'];
  }

  $json_array[] = $tree_id;
  return drupal_json_output($json_array);
}


/**
 *
 * @ingroup tripal_cv
 */
function tripal_cv_update_tree() {
  $content = array();
  $ontology = 'sequence';

  // get the id of the term to look up
  $cv = check_plain($_REQUEST['cv']);
  $term = check_plain($_REQUEST['term']);
  $tree_id = check_plain($_REQUEST['tree_id']);

  // get the options needed for this tree from the tripal module that
  // wants to create the tree
  $tripal_mod = preg_replace("/^(tripal_.+?)_cv_tree_(.+)$/", "$1", $tree_id);
  if ($tripal_mod) {
    $callback = $tripal_mod . "_cv_tree";
    $opt = call_user_func_array($callback, array($tree_id));
  }

  // get the CV root terms
  if (strcmp($term, 'root')==0) {
    if (!$cv) {
      $cv = $opt['cv_id'];
    }
    $content = tripal_cv_init_tree($cv, $opt['count_mview'],
      $opt['cvterm_id_column'], $opt['count_column'], $opt['filter'], $opt['label']);

  // get the children terms
  }
  else {
    $content = tripal_cv_get_term_children($term, $opt['count_mview'],
      $opt['cvterm_id_column'], $opt['count_column'], $opt['filter'], $opt['label']);
  }

  return drupal_json_output($content);
}

/**
 * Generates JSON needed for jsTree Root-level Branches
 *
 * This function returns the JSON array for the jsTree
 *    jQuery code that builds a tree for browsing the ontology.  This function
 *    should be called to generate the root level branches of the tree.
 *
 * @ingroup tripal_cv
 */
function tripal_cv_init_tree($cv_id, $cnt_table = NULL, $fk_column = NULL,
  $cnt_column = NULL, $filter = NULL, $label = NULL) {

  // get the list of root terms for the provided CV
  $sql = "
    SELECT *
    FROM {cv_root_mview} CRM
    WHERE cv_id = :cv_id
  ";
  $results = chado_query($sql, array(':cv_id' => $cv_id));

  // prepare the SQL statement that will allow us to pull out count
  // information for each term in the tree.
  if ($cnt_table) {
    if (!$filter) {
      $filter = '(1=1)';
    }
    $cnt_sql = "
       SELECT CVT.name, CVT.cvterm_id, CNT.$cnt_column as num_items
       FROM {" . $cnt_table . "} CNT
        INNER JOIN {cvterm} CVT on CNT.$fk_column = CVT.cvterm_id
       WHERE $filter AND CVT.cvterm_id = :cvterm_id
       ORDER BY $cnt_column desc
    ";
  }

  while ($term = $results->fetchObject()) {
    $name = $term->name;
    $count = 0;
    if ($cnt_table) {
      $cnt_results = chado_query($cnt_sql, array(':cvterm_id' => $term->cvterm_id));
      $count = $cnt_results->fetchObject()->num_items;

      if ($count > 0) {
        $name .= " ($count $label(s))";
      }
    }
    $content[] = array(
      'id' => $term->cvterm_id,
      'text' => $name,
      'state' => array(
          'opened' => false,
          'disabled' => false,
          'selected' => false,
      ),
      'children' => true,
      'type' => 'root'
    );
  }

  return $content;
}

/**
 *  Generates SON needed for jsTree -expanding a term to view children
 *
 *  This function returns the JSON array for the jsTree
 *    jQuery code when expanding a term to view it's children.
 *
 * @ingroup tripal_cv_api
 */
function tripal_cv_get_term_children($cvterm_id, $cnt_table = NULL,
    $fk_column = NULL, $cnt_column = NULL, $filter = NULL, $label = NULL) {

    // get the children for the term provided
    $sql = "
        SELECT CVTR.cvterm_relationship_id,CVTR.subject_id,
        CVT1.name as subject_name, CVT3.name as type_name, CVTR.type_id,
        CVT2.name as object_name,CVTR.object_id
        FROM {cvterm_relationship} CVTR
        INNER JOIN {CVTerm} CVT1 on CVTR.subject_id = CVT1.cvterm_id
        INNER JOIN {CVTerm} CVT2 on CVTR.object_id = CVT2.cvterm_id
        INNER JOIN {CVTerm} CVT3 on CVTR.type_id = CVT3.cvterm_id
        INNER JOIN {CV} on CV.cv_id = CVT1.cv_id
        WHERE CVTR.object_id = :cvterm_id
        ORDER BY CVT1.name
    ";

    $results = chado_query($sql, array('cvterm_id' => $cvterm_id));

    // prepare the SQL statement that will allow us to pull out count
    // information for each term in the tree.
    if ($cnt_table) {
        if (!$filter) {
            $filter = '(1=1)';
        }
        $cnt_sql = "
            SELECT CVT.name, CVT.cvterm_id, CNT.$cnt_column as num_items
            FROM {" . $cnt_table . "} CNT
            INNER JOIN {cvterm} CVT on CNT.$fk_column = CVT.cvterm_id
            WHERE $filter AND CVT.cvterm_id = :cvterm_id
            ORDER BY $cnt_column desc
        ";
    }

    // populate the JSON content array
    while ($term = $results->fetchObject()) {
        // count the number of items per term if requested
        $name = $term->subject_name;
        if ($cnt_table) {
            $count = 0;
            $cnt_results = chado_query($cnt_sql, array('cvterm_id' => $term->subject_id));
            $count = $cnt_results->fetchObject()->num_items;

            if ($count > 0) {
                $name .= " (" . number_format($count) . " $label)";
            }
        }

        // check if we have any children if so then set the value
        $children = chado_query($sql, array('cvterm_id' => $term->subject_id))->fetchObject();
        if ($children) {
            $children = true;
            $node_type = 'default';
        }
        else {
            $children = array();
            $node_type = 'leaf';
        }

        $content[] = array(
          'id' => $term->subject_id,
          'text' => $name,
          'state' => array(
              'opened' => false,
              'disabled' => false,
              'selected' => false,
          ),
          'children' => $children,
          'type' => $node_type,
        );
    }

    return $content;
}

function tripal_cv_cvterm_info($cvterm_id) {

  // get the id of the term to look up
  $cv = check_plain($_REQUEST['cv']);
  $tree_id = check_plain($_REQUEST['tree_id']);

  // first get any additional information to add to the cvterm
  if (strcmp($tree_id, 'undefined') != 0) {
    $tripal_mod = preg_replace("/^(tripal_.+?)_cv_tree_(.+)$/", "$1", $tree_id);
    if ($tripal_mod) {
      $callback = $tripal_mod . "_cvterm_add";
      $opt = call_user_func_array($callback, array($cvterm_id, $tree_id));
    }
  }

  $sql = "
    SELECT CVT.name as cvtermname, CVT.definition, CV.name as cvname,
       DBX.accession,DB.urlprefix,DB.db_id,DB.name as dbname
    FROM {CVTerm} CVT
      INNER JOIN {CV} on CVT.cv_id = CV.cv_id
      INNER JOIN {dbxref} DBX on CVT.dbxref_id = DBX.dbxref_id
      INNER JOIN {DB} on DBX.db_id = DB.db_id
    WHERE CVT.cvterm_id = :cvterm_id
  ";

  $cvterm = chado_query($sql, array('cvterm_id' => $cvterm_id))->fetchObject();

  $sql = "
    SELECT CVTS.synonym, CVT.name as cvname
    FROM {cvtermsynonym} CVTS
      INNER JOIN {cvterm} CVT on CVTS.type_id = CVT.cvterm_id
    WHERE CVTS.cvterm_id = :cvterm_id

  ";

  $results = chado_query($sql, array('cvterm_id' => $cvterm_id));

  $synonym_rows = '';
  while ($synonym = $results->fetchObject()) {
    $synonym_rows .= "<b>$synonym->cvname:</b>  $synonym->synonym<br />";
  }

  $accession = $cvterm->accession;
  if ($cvterm->urlprefix) {
    $accession = "<a href=\"$cvterm->urlprefix$cvterm->accession\">$cvterm->accession</a>";
  }

  $content = "
    <div id=\"cvterm\">
    <table>
      <tr><th>Term</th><td>$cvterm->cvtermname</td></tr>
      <tr><th>Accession</th><td><a href=\"http://amigo.geneontology.org/amigo/term/GO:$accession\"  target=\"_go\">GO:$accession</a></td></tr>
      <tr><th>Ontology</th><td>$cvterm->cvname</td></tr>
      <tr><th>Definition</th><td>$cvterm->definition</td></tr>
      <tr><th>Synonyms</th><td>$synonym_rows</td></tr>
      <tr><th>Internal ID</th><td>$cvterm_id</td></tr>
  ";

  // now add in any additional options from a hook
  if ($opt) {
    foreach ($opt as $key => $value) {
      $content .= "<tr><th>$key</th><td>$value</td>";
    }
  }

  // close out the information table
  $content .= "
    </table>
    </div>
  ";
  return drupal_json_output(array('update' => $content));
}

function tripal_analysis_go_cvterm_add($cvterm_id, $tree_id){
  $organism_id = preg_replace("/^tripal_analysis_go_cv_tree_(\d+)-(\d+)_(bp|cc|mf)$/","$1",$tree_id);
  $analysis_id = preg_replace("/^tripal_analysis_go_cv_tree_(\d+)-(\d+)_(bp|cc|mf)$/","$2",$tree_id);

  $sql = "
     SELECT DBX.accession
     FROM {cvterm} CVT
       INNER JOIN {dbxref} DBX on DBX.dbxref_id = CVT.dbxref_id
     WHERE cvterm_id = :cvterm_id
  ";

  $xref = chado_query($sql,array('cvterm_id' => $cvterm_id))->fetchObject();

  $link = url("download_goterm_features/$cvterm_id/$tree_id");
  $options = array(
     'Download sequences' => "<a href=\"$link\">GO_".$xref->accession.".fasta</a>",
  );
  return $options;
}

function tripal_analysis_go_get_goterm_features($cvterm_id,$tree_id){

   // get hte accession number for this cvterm and use it in naming the download
   $sql = "
      SELECT DBX.accession
      FROM {cvterm} CVT
        INNER JOIN {dbxref} DBX on DBX.dbxref_id = CVT.dbxref_id
      WHERE cvterm_id = :cvterm_id
   ";
   $xref = chado_query($sql,array(':cvterm_id' => $cvterm_id))->fetchObject();

   drupal_add_http_header('Content-Type', 'text');
   drupal_add_http_header('Content-Disposition', 'attachment; filename="GO_'.$xref->accession.'.fasta"');

   $organism_id = preg_replace("/^tripal_analysis_go_cv_tree_(\d+)-(\d+)_(bp|cc|mf)$/","$1",$tree_id);
   $analysis_id = preg_replace("/^tripal_analysis_go_cv_tree_(\d+)-(\d+)_(bp|cc|mf)$/","$2",$tree_id);
   $sql = "
      SELECT DISTINCT F.name,F.residues,F.feature_id
      FROM {cvtermpath} CVTP
         INNER JOIN {CVTerm} CVT1 on CVTP.subject_id = CVT1.cvterm_id
         INNER JOIN {CVTerm} CVT2 on CVTP.object_id = CVT2.cvterm_id
         INNER JOIN {AnalysisFeatureProp} AFP on AFP.type_id = CVTP.subject_id
         INNER JOIN {AnalysisFeature} AF on AF.analysisfeature_id = AFP.analysisfeature_id
         INNER JOIN {Feature} F on AF.feature_id = F.feature_id
      WHERE CVTP.object_id = :object_id and F.organism_id = :organism_id and AF.analysis_id = :analysis_id
      ORDER BY F.name
   ";
   $results = chado_query($sql,array(':object_id' => $cvterm_id, ':organism_id' => $organism_id, ':analysis_id' => $analysis_id));

   while($feature = $results->fetchObject()){
      $seq = tripal_get_feature_sequences(array('feature_id' => $feature->feature_id), array());
      print '>'.$seq[0]['defline']."\n";
      print $seq[0]['residues']."\n";
   }
   return;
}
