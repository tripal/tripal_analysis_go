<?php

$organism = $node->organism;
$form = $organism->tripal_analysis_go->select_form['form'];
$has_results = $organism->tripal_analysis_go->select_form['has_results'];

if ($has_results) { ?>
  <div id="tripal_organism-go_summary-box" class="tripal_organism-info-box tripal-info-box">
    <div  class="tripal_organism-info-box-title tripal-info-box-title">GO Analysis Reports</div>
    <?php print $form; ?>
    <div id="tripal_analysis_go_org_charts"></div>    
    <div id="tripal_cv_cvterm_info_box">
      <a href="#" onclick="$('#tripal_cv_cvterm_info_box').hide()" style="float: right">Close [X]</a>
      <div>Term Information</div>
      <div id="tripal_cv_cvterm_info"></div>
    </div>
    <div id="tripal_ajaxLoading" style="display:none">
      <div id="loadingText">Loading...</div>
    </div>
  </div> <?php
} 
else { 
  // show a message to the site administrator instructing how to enable 
  // a GO report.  Otherwise, if the user is not an administrator and 
  // there is no content then nothing get's shown.
  if (user_access('access administration pages')) { ?>
    <div id="tripal_organism-go_summary-box" class="tripal_organism-info-box tripal-info-box">
      <div  class="tripal_organism-info-box-title tripal-info-box-title">GO Analysis Reports</div>  
      <div class="tripal-no-results">
        There are no GO reports available
        <p><br>Administrators, to view a GO report you must:
        <ul>
          <li>Load GO assignments. GO assignments can be part of a GFF3 file and
             loaded using the <a href="<?php print url('admin/tripal/tripal_feature/gff3_load');?>">GFF loader</a> 
             or in a GAF format file and loaded using the <a href="<?php print url('admin/tripal/tripal_analysis_go/gaf_load');?>">GAF loader</a>.
             Tools such as Blast2GO are capable of assigning GO terms to features and generating GAF files. Additionally, the Tripal InterProScan Extension module parses InterProScan XML 
             output and imports GO terms associated with features. The InterProScan Extension Module is obtained separately 
             from the core Tripal package.</li>
          <li>Set the <a href="<?php print url('admin/tripal/tripal_cv/cvtermpath');?>">cvtermpath</a> for the 'biological process', 'molecular_function' and 'cellular_component' vocabularies. This should have been done automatically when the Gene Ontology was first loaded.</li>
          <li>Populate the <a href="<?php print url('admin/tripal/mviews');?>">go_count_analysis</a> materialized view</li>
          <li>Ensure the user <a href="<?php print url('admin/user/permissions'); ?>"> has permission</a> to view the GO analysis content</li>          
        </ul> 
        </p>
        This page will not appear to site visitors unless results are present.
      </div>
    </div>
    <?php
  }
}
