<?php

/**
 *
 *
 * @ingroup gaf_loader
 */
function tripal_analysis_go_load_gaf($analysis_id, $gaf_file, $gaf_file_ext, $seq_type,
   $query_uniquename, $method, $re_name, $job_id) {

  $transaction = db_transaction();
  print "\nNOTE: Loading of this GAF file is performed using a database transaction. \n" .
     "If the load fails or is terminated prematurely then the entire set of \n" .
     "insertions/updates is rolled back and will not be found in the database\n\n";

  print "Opening GAF file $gaf_file\n";

  try {

    // If user input a file (e.g. go.gaf)
    if (is_file($gaf_file)) {
        tripal_analysis_go_load_single_gaf($analysis_id, $gaf_file, $seq_type,
           $query_uniquename, $method, $re_name, $job_id);
    }
    // Otherwise, $gaf_file is a directory. Iterate through all xml files in it
    else {
      if (!$gaf_file_ext) {
         $gaf_file_ext = 'gaf';
      }

      // Parsing all files in the directory
      $dir_handle = @opendir($gaf_file) or die("Unable to open dir $gaf_file");
      $files_to_parse = [];
      while ($file = readdir($dir_handle)) {
        if (preg_match("/^.*\.$gaf_file_ext/i", $file)) {
          $files_to_parse[] = $file;
        }
      }

      $no_file = 0;
      $total_files = count($files_to_parse);
      foreach ($files_to_parse as $file) {
          print "File $no_file of $total_files: $file                       \n";
          tripal_analysis_go_load_single_gaf($analysis_id, "$gaf_file/$file", $seq_type,
               $query_uniquename, $method, $re_name, $job_id);
          $no_file ++;
      }
    }
  }
  catch (Exception $e) {
    print "\n"; // make sure we start errors on new line
    $transaction->rollback();
    watchdog_exception('T_GAF_load', $e);
    print "FAILED: Rolling back database changes...\n";
  }
  print "\nDone.\n";

  return;
}

function tripal_analysis_go_load_single_gaf($analysis_id, $gaf_file, $seq_type,
  $query_uniquename, $method, $re_name, $job_id) {

    $lines = file($gaf_file, FILE_SKIP_EMPTY_LINES);

    $name = '';
    $residues = '';
    $num_lines = sizeof($lines);
    $interval = intval($num_lines * 0.01);
    if ($interval == 0) {
        $interval = 1;
    }

    foreach ($lines as $line_num => $line) {
        // skip comments
        if(preg_match('/^\!/',$line)) {
            continue;
        }

        // update the job status every 1% features
        if($job_id and $line_num % $interval == 0) {
            tripal_set_job_progress($job_id, intval(($line_num/$num_lines)*100));
        }

        // split the line into it's columns
        $cols = explode("\t",$line);
        if (sizeof($cols) < 15) {
            print "ERROR: improper number of columns on line $line_num\n";
            print_r($cols);
            return '';
        }

        $db        = $cols[0];
        $object    = $cols[1];
        $symbol    = $cols[2];
        $qualifier = $cols[3];
        $go_id     = $cols[4];
        $dbxref    = $cols[5];
        $ecode     = $cols[6];
        $with      = $cols[7];
        $aspect    = $cols[8];
        $obj_name  = $cols[9];
        $obj_syn   = $cols[10];
        $obj_type  = $cols[11];
        $taxon     = $cols[12];
        $date      = $cols[13];
        $assigned  = $cols[14];
        $exten     = $cols[15];
        $product   = $cols[16];

        // get the name or uniquename for the feature
        $name = $object;
        if($re_name){
            if(!preg_match("/$re_name/",$object,$matches)){
                print "Regular expression for the feature name finds nothing\n";
            } else {
                $name = trim($matches[1]);
            }
        } else {
            if(preg_match("/^\s*(.*?)[\s\|].*$/",$object,$matches)){
                $name = trim($matches[1]);
            }
        }

        // get the feature
        $values = array(
            'type_id' => array(
                'cv_id' => array(
                    'name' => 'sequence'
                    ),
                'name' => $seq_type,
            )
        );
        if(!$query_uniquename){
            $values['name'] = $name;
        } else {
            $values['uniquename'] = $name;
        }
        $feature = chado_select_record('feature', array('*'), $values);
        if (count($feature) == 0) {
            print "WARNING: Cannot find the feature: '$name'\n";
        } else {
            // add the GO term to the feature
            tripal_analysis_go_load_gaf_go_term($feature[0], $go_id, $method, $analysis_id);
        }
    }
}

/**
*
*/
function tripal_analysis_go_load_gaf_go_term($feature, $dbxref, $method, $analysis_id){

   // get the database name from the reference.  If it doesn't exist then create one.
   $ref = explode(":",$dbxref);
   $dbname = $ref[0];
   $accession = $ref[1];

   // first look for the database name
   $db = chado_select_record('db', array('db_id'), array('name' => "DB:$dbname"));
   if(sizeof($db) == 0){
      $db = chado_select_record('db', array('db_id'), array('name' => "$dbname"));
   }
   if(sizeof($db) == 0){
      print "ERROR: Database, $dbname is missing for reference: $dbname:$accession\n";
      return 0;
   }

   $db = $db[0];

   // now check to see if the accession exists
   $dbxref = chado_select_record('dbxref', array('dbxref_id'), array(
      'accession' => $accession,'db_id' => $db->db_id));
   if(sizeof($dbxref) == 0){
      print "ERROR: Accession, $accession is missing for reference: $dbname:$accession\n";
      return 0;
   }
   $dbxref = $dbxref[0];

   // now check to see if the cvterm exists
   $cvterm = chado_select_record('cvterm', array('cvterm_id'), array(
      'dbxref_id' => $dbxref->dbxref_id));
   // if it doesn't exist in the cvterm table, look for an alternate id
   if(sizeof($cvterm) == 0){
      $cvterm = chado_select_record('cvterm_dbxref', array('cvterm_id'), array(
         'dbxref_id' => $dbxref->dbxref_id));
   }
   if(sizeof($cvterm) == 0){
      print "ERROR: CVTerm is missing for reference: $dbname:$accession\n";
      return 0;
   }
   $cvterm = $cvterm[0];


   // check to see if this feature cvterm already exists
   $fcvt = chado_select_record('feature_cvterm',array('feature_cvterm_id'),
      array('cvterm_id' => $cvterm->cvterm_id,'feature_id' => $feature->feature_id));

   // now associate this feature with the cvterm if it doesn't already exist
   if(sizeof($fcvt)==0){
      $values = array(
         'feature_id' => $feature->feature_id,
         'cvterm_id' => $cvterm->cvterm_id,
         'pub_id' => array(
            'uniquename' => 'null',
         ),
      );
      $ret = chado_insert_record('feature_cvterm', $values);

      if($ret){
         print "   Added ontology term $dbname:$accession to feature $feature->uniquename\n";
      } else {
         print "ERROR: failed to insert ontology term '$dbname:$accession' for feature: $feature\n";
         return 0;
      }
   } else {
      if($method == 'Remove GO terms'){
         $status = tripal_core_chado_delete('feature_cvterm',
            array('cvterm_id' => $cvterm->cvterm_id,
                 'feature_id' => $feature->feature_id));
         if(!$status){
            print "ERROR: Failed to delete ontology term $dbname:$accession from feature $feature->uniquename\n";
         } else {
            print "   Deleted ontology term $dbname:$accession from feature $feature->uniquename\n";
         }
      } else {
         print "   Ontology term already associated to feature $feature->uniquename, skipping $dbname:$accession\n";
      }
   }

   if($method != 'Remove GO terms'){
      print "   Associating feature $feature->name to analysis\n";

      // Insert into analysisfeature table only if it doesn't already exist
      $values = array('feature_id' => $feature->feature_id, 'analysis_id' => $analysis_id);
      $analysisfeature = chado_select_record('analysisfeature',array('*'),$values);
      if(sizeof($analysisfeature) == 0){
         $analysisfeature = chado_insert_record('analysisfeature',$values);
         $analysisfeature_id = $analysisfeature['analysisfeature_id'];
      } else {
         $analysisfeature_id = $analysisfeature[0]->analysisfeature_id;
      }

      // Insert GO terms into analysisfeatureprop table
      $values = array('analysisfeature_id' => $analysisfeature_id,
                      'type_id' => $cvterm->cvterm_id,
                      'value' => "$dbname:$accession",
                      'rank' => 0);
      $analysisfeatureprop = chado_select_record('analysisfeatureprop',array('*'),$values);
      if(sizeof($analysisfeatureprop) == 0){
         $analysisfeatureprop = chado_insert_record('analysisfeatureprop',$values);
      }
   }
   return 1;
}
