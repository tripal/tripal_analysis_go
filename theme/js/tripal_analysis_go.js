(function ($) {

// Slightly modified version of code from drupal/misc/ajax.js to allow calling function without associate
Drupal.ajax.prototype.commands.invoke = function (ajax, response, status) {
    var $element = $(response.selector);
    if ($element.length == 0) {
        $.fn[response.method]($element, response.arguments);
    }
    else {
        $element[response.method].apply($element, response.arguments);
    }
  },

google.charts.load('current', {'packages':['corechart']});

$.fn.tripal_analysis_go_init_graph = function() {
    baseurl = Drupal.settings.basePath;

    $('.tripal_cv_chart').each(function(index) {

        $.ajax({
            'url': baseurl + '/tripal_cv_chart/'+this.id,
            'dataType': 'json',
            'type': 'POST',
            success: function(data){
                var graph_id = data[1];
                var content = data[0]

                var arrayData = [
                    ['Number of terms', ''],
                ];

                for (index = 0; index < content.axis_labels.length; ++index) {
                    arrayData.push([content.axis_labels[index], content.data[index][0]]);
                }

                var data = google.visualization.arrayToDataTable(arrayData);

                var options = {
                  title: content.title
                };

                var chart = new google.visualization.PieChart($('#'+graph_id)[0]);

                chart.draw(data, options);
            },
        });
    });
}


$.fn.tripal_analysis_go_init_tree = function() {
    baseurl = Drupal.settings.basePath;

    $('.tripal_cv_tree').each(function(index) {

        // Get some info to create the tree
         $.ajax({
            'url': baseurl + '/tripal_cv_tree/'+this.id,
            'dataType': 'json',
            'type': 'POST',
            success: function(data){
                 vars = {
                  'cv' : data[0],
                  'tree_id' : data[1],
                 }

                 $('#'+vars.tree_id).on('select_node.jstree', function (node, data, e) {
                        tripal_cv_cvterm_info( data.selected[0], vars );
                    })
                 .jstree({
                   "core" : {
                     "animation" : 0,
                     "check_callback" : false,
                     "themes" : { "stripes" : true },
                     'data' : {
                       'url': function(node) {
                           return baseurl + '/tripal_cv_update_tree/'+this.element.attr('id');
                       },
                       'data': function(node) {
                           vars.term = "root";
                           if (node.id != '#')
                               vars.term = node.id;
                           return vars;
                       }
                     }
                   },
                   "types" : {
                     "#" : {
                       "max_children" : 1,
                       "valid_children" : ["root"]
                     },
                     "root" : {
                       "valid_children" : ["default"]
                     },
                     "default" : {
                       "valid_children" : ["default", "leaf"]
                     },
                     "leaf" : {
                       "icon" : "jstree-file",
                       "valid_children" : []
                     }
                   },
                   "plugins" : [
                     "types"
                   ]
                 });
            }
         });
    })
};

function tripal_cv_cvterm_info(cvterm_id, vars){
  var link = Drupal.settings.basePath;
  link += '/tripal_cv_cvterm_info/' + cvterm_id + '?cv=' + vars.cv + '&tree_id=' + vars.tree_id;

  // Get the cv_id from DOM
  $.ajax({
     url: link,
     dataType: 'json',
     type: 'POST',
     success: function(data){
        $("#tripal_cv_cvterm_info").html(data.update);
        $('#tripal_cv_cvterm_info_box').animate(
            {top:$(window).scrollTop()+"px" },
            {queue: false, duration: 350}
        );
        $("#tripal_cv_cvterm_info_box").show();
     }
  });
  return false;
}
})(feature_viewer_jquery);
