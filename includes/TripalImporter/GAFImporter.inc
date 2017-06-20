<?php

class GAFImporter extends TripalImporter {

  /**
   * The name of this loader.  This name will be presented to the site
   * user.
   */
  public static $name = 'Chado GAF Loader';

  /**
   * The machine name for this loader. This name will be used to construct
   * the URL for the loader.
   */
  public static $machine_name = 'chado_gaf_loader';

  /**
   * A brief description for this loader.  This description will be
   * presented to the site user.
   */
  public static $description = 'Import a GFF3 file into Chado (currently only supports importing of GO associations)';

  /**
   * An array containing the extensions of allowed file types.
   */
  public static $file_types = array();


  /**
   * Provides information to the user about the file upload.  Typically this
   * may include a description of the file types allowed.
   */
  public static $upload_description = 'Please provide the GAF file.';

  /**
   * The title that should appear above the upload button.
   */
  public static $upload_title = 'GAF File';

  /**
   * If the loader should require an analysis record.  To maintain provenance
   * we should always indiate where the data we are uploading comes from.
   * The method that Tripal attempts to use for this by associating upload files
   * with an analysis record.  The analysis record provides the details for
   * how the file was created or obtained. Set this to FALSE if the loader
   * should not require an analysis when loading. if $use_analysis is set to
   * true then the form values will have an 'analysis_id' key in the $form_state
   * array on submitted forms.
   */
  public static $use_analysis = TRUE;

  /**
   * If the $use_analysis value is set above then this value indicates if the
   * analysis should be required.
   */
  public static $require_analysis = TRUE;

  /**
   * Text that should appear on the button at the bottom of the importer
   * form.
   */
  public static $button_text = 'Import File';

  /**
   * Indicates the methods that the file uploader will support.
   */
  public static $methods = array(
    // Allow the user to upload a file to the server.
    'file_upload' => TRUE,
    // Allow the user to provide the path on the Tripal server for the file.
    'file_local' => TRUE,
    // Allow the user to provide a remote URL for the file.
    'file_remote' => TRUE,
  );

  /**
   * Indicates if the file must be provided.  An example when it may not be
   * necessary to require that the user provide a file for uploading if the
   * loader keeps track of previous files and makes those available for
   * selection.
   */
  public static $file_required = TRUE;


  /**
   * The array of arguments used for this loader.  Each argument should
   * be a separate array containing a machine_name, name, and description
   * keys.  This information is used to build the help text for the loader.
   */
  public static $argument_list = array();


  /**
   * @see TripalImporter::form()
   */
  public function form($form, &$form_state) {
    // set the defaults
    $gaf_file_ext = '';
    $seq_type = '';
    $query_type = 'uniquename';
    $re_name = '';
    $name_col = '2';

    // if we are re constructing the form from a failed validation or ajax callback
    // then use the $form_state['values'] values
    if (array_key_exists('values', $form_state)) {
      $gaf_file_ext = $form_state['values']['gaf_file_ext'];
      $seq_type = $form_state['values']['seq_type'];
      $query_type = array_key_exists('query_type', $form_state['values']) ? $form_state['values']['query_type'] : $query_type;
      $re_name = $form_state['values']['re_name'];
      $re_name = $form_state['values']['re_name'];
      $name_col = $form_state['values']['name_col'];
    }
    // if we are re building the form from after submission (from ajax call) then
    // the values are in the $form_state['input'] array
    if (array_key_exists('input', $form_state) and !empty($form_state['input'])) {
      $gaf_file_ext = $form_state['input']['gaf_file_ext'];
      $seq_type = $form_state['input']['seq_type'];
      $query_type = array_key_exists('query_type', $form_state['input']) ? $form_state['input']['query_type'] : $query_type;
      $re_name = $form_state['input']['re_name'];
      $name_col = $form_state['input']['name_col'];
    }

    // get the list of organisms
    $sql = "SELECT * FROM {organism} ORDER BY genus, species";
    $org_rset = chado_query($sql);
    $organisms = array();
    $organisms[''] = '';
    while ($organism = $org_rset->fetchObject()) {
      $organisms[$organism->organism_id] = "$organism->genus $organism->species ($organism->common_name)";
    }
    $form['organism_id'] = array(
      '#title' => t('Organism'),
      '#type' => t('select'),
      '#description' => t("Choose the organism to which these sequences are associated"),
      '#required' => TRUE,
      '#options' => $organisms
    );

   $cv = tripal_get_cv(array('name' => 'sequence'));
   $cv_id = $cv->cv_id;
   $form['seq_type']= array(
        '#title' => t('Sequence Type'),
        '#type' => 'textfield',
        '#description' => t('Please enter the Sequence Ontology term that describes '.
            'the query sequences used for blasting.  This is only necessary if two '.
            'or more sequences have the same name.'),
        '#default_value' => $seq_type,
        '#autocomplete_path' => "admin/tripal/storage/chado/auto_name/cvterm/$cv_id",
    );
    $form['query_type'] = array(
        '#title' => t('Name Matching'),
        '#type' => 'radios',
        '#options' => array(
          'name' => 'Feature Name',
          'uniquename' => 'Feature Unique Name',
          'dbxref' => 'Cross Reference',
        ),
        '#description' => t('Select how this loader will match names. Chado stores
            features using a name and a unique name. The unique name is usually
            the unique idnetifier for the feature and is guaranteed to be
            unique for the species and sequence type.  Alternatively, if
            the features are referred to with a different database\'s
            unique accession numbers then choose "Cross Reference".'),
        '#default_value' => $query_type,
    );

    $form['name_col'] = array(
      '#title' => t('Name Column'),
      '#type' => 'select',
      '#options' => array(
        '2' => 'DB Object ID (2nd column)',
        '3' => 'DB Object Symbol (3rd column)',
        '10' => 'DB Object Name (10th column)',
        '11' => 'DB Object Synonym (11th column)',
      ),
      '#description' => t('Please select the column in the GAF file that
          contains the names of the feature to which GO terms are mapped.'),
      '#default_value' => $name_col,
    );

    $form['re_name']= array(
      '#type' => 'textfield',
      '#title' => t('Name Regular Expression'),
      '#required' => FALSE,
      '#description' => t('Enter the regular expression that will extract the
         feature name from the GAF file. This option is
         is only required when the feature identifier does not identically match a feature
         in the database. A regular expression is an advanced method for extracting information from a string of text.
         By default, this loader will use the first word in the second column of the GAF file
         as the uniquename for the sequences.  If this is not desired, you may use the following regular
         expressions to define the location of the name or unique name within the text of column 2.'),
     '#default_value' => $re_name,
    );

    $form['gaf_file_ext']= array(
      '#type'          => 'textfield',
      '#title'         => t('GAF File extension'),
      '#description'   => t('If a directory is provide for the GAF file setting above,
        then a file extension can be provided here. Files with this extension
        in the directory will be parsed. If no extension is provided then files
        with a .gaf extension will be parsed within the directory.
        Please provide the extension without the preceeding period (e.g. "gaf"
        rather than ".gaf"'),
      '#required' => FALSE,
      '#default_value' => $gaf_file_ext,
    );


    return $form;
  }

  /**
   * @see TripalImporter::formSubmit()
   */
  public function formSubmit($form, &$form_state) {

  }

  /**
   * @see TripalImporter::formValidate()
   */
  public function formValidate($form, &$form_state) {
    $gaf_file_ext = trim($form_state['values']['gaf_file_ext']);
    $seq_type = trim($form_state['values']['seq_type']);
    $query_type = trim($form_state['values']['query_type']);
    $re_name  = trim($form_state['values']['re_name']);
    $organism_id  = trim($form_state['values']['organism_id']);
    $name_col  = trim($form_state['values']['name_col']);

    if (!in_array($name_col, array(2,3,10,11))) {
      form_set_error('name_col', "Please provide a valid name column.");
    }
  }

  /**
   * @see TripalImporter::run()
   */
  public function run() {
    $arguments = $this->arguments['run_args'];
    $gaf_file = $this->arguments['file']['file_path'];

    // Get the importer arguments.
    $analysis_id = $this->arguments[run_args]['analysis_id'];
    $organism_id = $this->arguments[run_args]['organism_id'];
    $gaf_file_ext = $this->arguments[run_args]['gaf_file_ext'];
    $seq_type  = $this->arguments[run_args]['seq_type'];
    $query_type = $this->arguments[run_args]['query_type'];
    $re_name = $this->arguments[run_args]['re_name'];
    $name_col = $this->arguments[run_args]['name_col'];

    if (is_dir($gaf_file)) {
      $this->loadMultiGAF($analysis_id, $organism_id, $file_path, $gaf_file_ext,
        $seq_type, $query_type, $name_col, $re_name);
    }
    else {
      $this->loadGAF($analysis_id, $organism_id, $gaf_file, $seq_type,
        $query_type, $name_col, $re_name);
    }
  }

  /**
   * Imports multiple GAF files in a single directory.
   *
   * @param $analysis_id
   * @param $gaf_file
   * @param $gaf_file_ext
   * @param $seq_type
   * @param $query_type
   * @param $method
   * @param $re_name
   */
  private function loadMultiGAF($analysis_id, $organism_id, $gaf_file,
      $gaf_file_ext, $seq_type, $query_type, $re_name) {

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
      $this->logMessage("File $no_file of $total_files: $file");
      $this->loadGAF($analysis_id, $organism_id, "$gaf_file/$file", $seq_type,
          $query_type, $name_col, $re_name);
      $no_file ++;
    }
  }

  /**
   * Imports a Single GAF file.
   *
   * @param $analysis_id
   * @param $gaf_file
   * @param $seq_type
   * @param $query_type
   * @param $re_name
   */
  private function loadGAF($analysis_id, $organism_id, $gaf_file, $seq_type,
      $query_type, $name_col, $re_name) {

    $lines = file($gaf_file, FILE_SKIP_EMPTY_LINES);

    $name = '';
    $residues = '';
    $num_lines = sizeof($lines);
    $this->setTotalItems($num_lines);

    foreach ($lines as $line_num => $line) {
      $this->setItemsHandled($line_num);

      // skip comments
      if(preg_match('/^\!/',$line)) {
        continue;
      }

      // split the line into it's columns
      $cols = explode("\t",$line);
      if (sizeof($cols) < 15) {
        throw new Exception("Improper number of columns on line $line_num");
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

      // Get the name, uniquename or dbxref accession for the feature.
      $name = $cols[$name_col-1];
      if($re_name){
        if(!preg_match("/$re_name/", $object, $matches)){
          $this->logMessage("Regular expression for the feature name finds nothing: '$object'");
        }
        else {
          $name = trim($matches[1]);
        }
      }
      else {
        if(preg_match("/^\s*(.*?)[\s\|].*$/", $object, $matches)){
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
        ),
        'organism_id' => $organism_id,
      );
      if ($query_type == 'name') {
        $values['name'] = $name;
      }
      if ($query_type == 'uniquename') {
        $values['uniquename'] = $name;
      }
      if ($query_type == 'dbxref') {
        // To find a matching record by a database cross reference, it's
        // easiest just to write out the big long SQL.
        $sql = "
           SELECT FDBX.feature_id
           FROM {feature_dbxref} FDBX
             INNER JOIN {dbxref} DBX on FDBX.dbxref_id = DBX.dbxref_id
             INNER JOIN {feature} F on F.feature_id = FDBX.feature_id
             INNER JOIN {cvterm} CVT on F.type_id = CVT.cvterm_id
             INNER JOIN {cv} CV on CV.cv_id = CVT.cv_id
           WHERE
            DBX.accession = :accession and
            F.organism_id = :organism_id and
            CV.name = 'sequence' and
            CVT.name = :seq_type
        ";
        $args = array(
          ':accession' => $name,
          ':organism_id' => $organism_id,
          ':seq_type' => $seq_type,
        );
        $result = chado_query($sql, $args);
        $feature_id = $result->fetchField();
        $values = array(
          'feature_id' => $feature_id
        );
        if (!$feature_id) {
          $this->logMessage('Cannot find a feature with a database cross reference of, "!ref", for the given organism and feature type.', array('!ref' => $name));
          continue;
        }
      }

      $feature = chado_select_record('feature', array('*'), $values);

      if (count($feature) == 0) {
        $this->logMessage("Cannot find the feature: '$name'");
      }
      else {
        // add the GO term to the feature
        $this->loadGOTerm($feature[0], $go_id, $analysis_id);
      }
    }
  }

  /**
   *
   * @param $feature
   * @param $dbxref
   * @param $analysis_id
   *
   * @throws Exception
   */
  private function loadGOTerm($feature, $dbxref, $analysis_id){

    // Get the database name from the reference.  If it doesn't exist then
    // create one.
    $ref = explode(":",$dbxref);
    $dbname = $ref[0];
    $accession = $ref[1];

    // First look for the database name.
    $db = chado_select_record('db', array('db_id'), array('name' => "DB:$dbname"));
    if(sizeof($db) == 0){
      $db = chado_select_record('db', array('db_id'), array('name' => "$dbname"));
    }
    if(sizeof($db) == 0){
      throw new Exception("Database, $dbname is missing for reference: $dbname:$accession");
    }

    $db = $db[0];

    // Now check to see if the accession exists.
    $dbxref = chado_select_record('dbxref', array('dbxref_id'), array(
      'accession' => $accession,'db_id' => $db->db_id));
    if(sizeof($dbxref) == 0){
      throw new Exception("Accession, $accession is missing for reference: $dbname:$accession");
    }
    $dbxref = $dbxref[0];

    // Now check to see if the cvterm exists.
    $cvterm = chado_select_record('cvterm', array('cvterm_id'), array(
      'dbxref_id' => $dbxref->dbxref_id));
    // if it doesn't exist in the cvterm table, look for an alternate id
    if(sizeof($cvterm) == 0){
      $cvterm = chado_select_record('cvterm_dbxref', array('cvterm_id'), array(
        'dbxref_id' => $dbxref->dbxref_id));
    }
    if(sizeof($cvterm) == 0){
      throw new Exception("CVTerm is missing for reference: $dbname:$accession");
    }
    $cvterm = $cvterm[0];


    // Check to see if this feature cvterm already exists.
    $fcvt = chado_select_record('feature_cvterm',array('feature_cvterm_id'),
        array('cvterm_id' => $cvterm->cvterm_id, 'feature_id' => $feature->feature_id));

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
        $this->logMessage("Added ontology term $dbname:$accession to feature $feature->uniquename");
      }
      else {
        throw new Exception("Failed to insert ontology term '$dbname:$accession' for feature: $feature");
      }
    }
    else {
      $this->logMessage("Ontology term already associated to feature $feature->uniquename, skipping $dbname:$accession");
    }

    // Insert into analysisfeature table only if it doesn't already exist
    $values = array('feature_id' => $feature->feature_id, 'analysis_id' => $analysis_id);
    $analysisfeature = chado_select_record('analysisfeature',array('*'),$values);
    if(sizeof($analysisfeature) == 0){
      $analysisfeature = chado_insert_record('analysisfeature',$values);
      $analysisfeature_id = $analysisfeature['analysisfeature_id'];
    }
    else {
      $analysisfeature_id = $analysisfeature[0]->analysisfeature_id;
    }

    // Insert GO terms into analysisfeatureprop table
    $values = array('analysisfeature_id' => $analysisfeature_id,
      'type_id' => $cvterm->cvterm_id,
      'value' => "$dbname:$accession",
      'rank' => 0);
    $analysisfeatureprop = chado_select_record('analysisfeatureprop',array('*'),$values);
    if(sizeof($analysisfeatureprop) == 0){
      $this->logMessage("Associating feature $feature->uniquename to analysis $analysis_id");
      $analysisfeatureprop = chado_insert_record('analysisfeatureprop',$values);
    }
  }

}