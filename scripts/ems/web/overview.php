<?php

date_default_timezone_set('Europe/London');

function setup() {
  $setup = file("setup");
  head("All Experimental Setups");
  print "<table border=1 cellpadding=1 cellspacing=0>\n";
  print "<TR><TD>ID</TD><TD>User</TD><TD>Task</TD><TD>Directory</TD></TR>\n";
  $rev = array_reverse($setup);
  while (list($i,$line) = each($rev)) {
    $dir = explode(";",$line);
    print "<TR><TD><A HREF=\"?setup=$dir[0]\">$dir[0]</A></TD><TD>$dir[1]</TD><TD>$dir[2]</TD><TD>$dir[3]</TD></TR>\n";
  }
  print "</TABLE>\n";
  print "<P>To add experiment, edit /fs/thor4/html/experiment/setup";
}

function overview() {
  global $evalset;
  global $experiment,$comment;
  global $task,$user,$setup;
  global $dir;
  global $has_analysis;
  $has_analysis = array();

  head("Task: $task ($user)");
  print "<a href=\"http://www.statmt.org/wiki/?n=Experiment.$setup\">Wiki Notes</a>";
  print " &nbsp; &nbsp; | &nbsp; &nbsp; <a href=\"/\">Overview of experiments</a> &nbsp; &nbsp; | &nbsp; &nbsp; <code>$dir</code><p>";
  reset($experiment);

  print "<form action=\"\" method=get>\n";
  output_state_for_form();

  // count how many analyses there are for each test set
  while (list($id,$info) = each($experiment)) {
    reset($evalset);
    while (list($set,$dummy) = each($evalset)) {
      $analysis = "$dir/evaluation/$set.analysis.$id";
      $report_info = "$dir/steps/$id/REPORTING_report.$id.INFO";
      // does the analysis file exist?
      if (file_exists($analysis)) {
	if (!array_key_exists($set,$has_analysis)) { 
	  $has_analysis[$set] = 0;
        }
        $has_analysis[$set]++;
      }
    }
  }
  reset($experiment);

  print "<table border=1 cellpadding=1 cellspacing=0>
<tr><td><input type=submit name=diff value=\"compare\"></td>
    <td align=center>ID</td>
    <td align=center>start</td>
    <td align=center>end</td>\n";
  reset($evalset);

  while (list($set,$dummy) = each($evalset)) {
    if (array_key_exists($set,$has_analysis)) {
      print "<td align=center colspan=2>";
      if ($has_analysis[$set]>=2) {
        print " <input type=submit name=\"analysis_diff_home\" value=\"$set\">";
      }
      else {
        print $set;
      }
      print "</td>";
    }
    else {
      print "<td align=center>$set</td>";
    }
  }
  print "</tr>\n";
  while (list($id,$info) = each($experiment)) {
    $state = return_state_for_link();
    print "<tr id=\"row-$id\" onMouseOver=\"highlightLine($id);\" onMouseOut=\"highlightBest();\"><td><input type=checkbox name=run[] value=$id><a href=\"?$state&show=config.$id\">cfg</a>|<a href=\"?$state&show=parameter.$id\">par</a>|<a href=\"?$state&show=graph.$id.png\">img</a></td><td><span id=run-$setup-$id><a href='javascript:createCommentBox(\"$setup-$id\");'>[$setup-$id]</a>";
    if (array_key_exists("$setup-$id",$comment)) {
      print " ".$comment["$setup-$id"]->name;
    }
    print "</span></td><td align=center>".mytime($info->start,0)."</td><td align=center>";

    if (mytime($info->end,1) == "running") {
      print "<font size=-2>".$info->last_step;
      if ($info->last_step == "TUNING<BR>tune") {
        print "<BR>".tune_status($id);
      }
      else if ($info->last_step == "TRAINING<BR>run-giza" ||
               $info->last_step == "TRAINING<BR>run-giza-inverse" ||
	       preg_match('/EVALUATION.+decode/',$info->last_step,$dummy) ||
               $info->last_step == "TRAINING<BR>extract-phrases") {
	$module_step = explode("<BR>",$info->last_step);
        $step_file = "$dir/steps/$id/$module_step[0]_$module_step[1].$id";
	print "<BR><span id='$module_step[0]-$module_step[1]-$id'><img src=\"spinner.gif\" width=12 height=12></span>";
?><script language="javascript" type="text/javascript">
new Ajax.Updater("<?php print "$module_step[0]-$module_step[1]-$id"; ?>", '?setStepStatus=' + encodeURIComponent("<?php print $step_file; ?>"), { method: 'get', evalScripts: true });</script>
<?php
      }
    }
    else if (property_exists($info,"result")) {
      print mytime($info->end,1);

      print "<br><font size=-2>";
      print dev_score("$dir/tuning/moses.ini.$id");
    }
    else {
      print "<font color=red>crashed";
    }

    print "</td>";

    output_score($id,$info);

    print "</tr>\n";
  }
  print "</table>";
  print "<script language=\"javascript\" type=\"text/javascript\">\n";
  print "var currentComment = new Array();\n";
  reset($experiment);
  while (list($id,$info) = each($experiment)) {
    if (array_key_exists("$setup-$id",$comment)) {
      print "currentComment[\"$setup-$id\"] = \"".$comment["$setup-$id"]->name."\";\n";
    }
  }
  reset($experiment);
  $best = array();
  print "var score = [];\n";
  while (list($id,$info) = each($experiment)) {
      reset($evalset);
      print "score[$id] = [];\n";
      while (list($set,$dummy) = each($evalset)) {
	  if (property_exists($info,"result") &&
	      array_key_exists($set,$info->result)) {
	      list($score) = sscanf($info->result[$set],"%f%s");
	      if ($score > 0) {
	        print "score[$id][\"$set\"] = $score;\n";
		if (!array_key_exists($set,$best) || $score > $best[$set]) {
		    $best[$set] = $score;
		}
	      }
	  }
	  else { $score = ""; }  
      }
  }
  print "var best_score = [];\n";
  reset($evalset);
  while (list($set,$dummy) = each($evalset)) {
      if ($best[$set] != "" && $best[$set]>0) {
        print "best_score[\"$set\"] = ".$best[$set].";\n";
      }
  }
?>

// Get the HTTP Object
function getHTTPObject(){
  if (window.ActiveXObject) return new ActiveXObject("Microsoft.XMLHTTP");
  else if (window.XMLHttpRequest) return new XMLHttpRequest();
  else {
    alert("Your browser does not support AJAX.");
    return null;
  }
} 
function createCommentBox( runID ) {
  document.getElementById("run-" + runID).innerHTML = "<form onsubmit=\"return false;\"><input id=\"comment-" + runID + "\" name=\"comment-" + runID + "\" size=30><br><input type=submit onClick=\"addComment('" + runID + "');\" value=\"Add Comment\"></form>"; 
  if (currentComment[runID]) {
    document.getElementById("comment-" + runID).value = currentComment[runID];
  }
  document.getElementById("comment-" + runID).focus();
}

var httpObject = null;

function addComment( runID ) {
  httpObject = null;
  httpObject = getHTTPObject();
  if (httpObject != null) {
    httpObject.onreadystatechange = setComment;
    httpObject.open("GET", "comment.php?run="+encodeURIComponent(runID)+"&text="
                    +encodeURIComponent(document.getElementById('comment-'+runID).value), true);
    httpObject.send(null);
    currentComment[runID] = document.getElementById('comment-'+runID).value;
    document.getElementById("run-" + runID).innerHTML = "<a href='javascript:createCommentBox(\"" + runID + "\");'>[" + runID + "]</a> " + document.getElementById('comment-'+runID).value;
  }
  return true;
}

function setComment() {
  if(httpObject.readyState == 4){
    //alert("c:" +httpObject + httpObject.status + " " + httpObject.responseText);
    httpObject = null;
  }
}

function highlightBest() {
    lowlightAll();
    for (set in best_score) {
	for (run in score) {
	    var column = "score-"+run+"-"+set;
	    if ($(column)) {
	        if (score[run][set] == best_score[set]) {		
		   $(column).setStyle({ backgroundColor: '#a0ffa0'});
		}
	        else if (score[run][set]+1 >= best_score[set]) {
		   $(column).setStyle({ backgroundColor: '#e0ffe0'});
		}
	    }
	}
    }
}

function highlightLine( id ) {
  lowlightAll();
  var row = "row-"+id;
  $(row).setStyle({ backgroundColor: '#f0f0f0'});
  for (set in score[id]) {
    for (run in score) {
      var column = "score-"+run+"-"+set;
      if ($(column)) {
        if (run == id) {
          $(column).setStyle({ backgroundColor: '#ffffff'});
        }
        else {
	  if (score[run][set] < score[id][set]-1) {		
	    $(column).setStyle({ backgroundColor: '#ffa0a0'});
	  }
	  else if (score[run][set] < score[id][set]) {
	    $(column).setStyle({ backgroundColor: '#ffe0e0'});
	  }
          else if (score[run][set] > score[id][set]+1) {
	    $(column).setStyle({ backgroundColor: '#a0ffa0'});
	  }
          else if (score[run][set] > score[id][set]) {
	    $(column).setStyle({ backgroundColor: '#e0ffe0'});
	  }
	}
      }
    }
  }   
}
function lowlightAll() {
  for (run in score) {
    var row = "row-"+run
    if ($(row)) {
      $(row).setStyle({ backgroundColor: 'transparent' });
    }
    for (set in best_score) {
      var column = "score-"+run+"-"+set;
      if ($(column)) {
	$(column).setStyle({ backgroundColor: 'transparent' });
      }
    }
  }
}

highlightBest();
//-->
</script>
<?php
}

function mytime($timestamp,$running) {
  if ($running && $timestamp + 300 > time()) {
    return "running";
  }
  if ($timestamp + 12*3600 > time()) {
    return strftime("%T",$timestamp);
  }
  if ($timestamp + 5*24*3600 > time()) {
    return strftime("%a %H:%M",$timestamp);
  }
  if ($timestamp + 180*24*3600 > time()) {
    return strftime("%d %b",$timestamp);
  }
  return strftime("%d %b %g",$timestamp);
}

function output_score($id,$info) {
  global $evalset;
  global $has_analysis;
  global $setup;
  global $dir;
  reset($evalset);
  $state = return_state_for_link();

  while (list($set,$dummy) = each($evalset)) {
    if (property_exists($info,"result") &&
        array_key_exists($set,$info->result)) {
      $score = $info->result[$set];
    }
    else { $score = ""; }
    print "<td align=center id=\"score-$id-$set\">";

    // print "<table><tr><td>";

    $each_score = explode(" ; ",$score);
    for($i=0;$i<count($each_score);$i++) {
      if (preg_match('/([\d\(\)\.\s]+) (BLEU[\-c]*)/',$each_score[$i],$match) ||
          preg_match('/([\d\(\)\.\s]+) (IBM[\-c]*)/',$each_score[$i],$match)) {
        if ($i>0) { print "<BR>"; }
	$opened_a_tag = 0;
        if ($set != "avg") { 
	  if (file_exists("$dir/evaluation/$set.cleaned.$id")) {
	    print "<a href=\"?$state&show=evaluation/$set.cleaned.$id\">"; 
            $opened_a_tag = 1;
	  }
          else if (file_exists("$dir/evaluation/$set.output.$id")) {
            print "<a href=\"?$state&show=evaluation/$set.output.$id\">"; 
	    $opened_a_tag = 1;
          }
        }
        if ($set == "avg" && count($each_score)>1) { print $match[2].": "; }
        print $match[1];
        if ($opened_a_tag) { print "</a>"; }
      }
      else {
        print "-";
      }
    }

    print "</td>";
    if ($has_analysis && array_key_exists($set,$has_analysis)) {
      print "<td align=center>";
      global $dir;
      $analysis = get_analysis_version($dir,$set,$id);
      if ($analysis["basic"]) {
        print "<a href=\"?analysis=show&setup=$setup&set=$set&id=$id\">&#x24B6;</a> <input type=checkbox name=analysis-$id-$set value=1>";
      }
      print "</td>";
    }
  }
}

function tune_status($id) {
  global $dir;
  $max_iteration = 0;
  if (! file_exists($dir."/tuning/tmp.".$id)) { return ""; }
  $d = dir($dir."/tuning/tmp.".$id);
  while (false !== ($entry = $d->read())) {
    if (preg_match('/run(\d+).moses.ini/',$entry,$match) 
        && $match[1] > $max_iteration) {
      $max_iteration = $match[1];
    }
  }
  if ($max_iteration <= 1) { return $max_iteration; }
  return dev_score("$dir/tuning/tmp.$id/run$max_iteration.moses.ini");
}

function dev_score($moses_ini) {
  if (! file_exists($moses_ini)) { return ""; }
  $moses_ini = file($moses_ini);
  $last_iter = -1;
  $info = "";
  while (list($id,$line) = each($moses_ini)) {
    if (preg_match('/# BLEU ([\d\. \-\>]+)/',$line,$match)) {
      $info = $match[1];
    }
    if (preg_match('/# We were before running iteration (\d+)/',$line,$match)) {
      $last_iter = $match[1];
    }
  }
  if ($last_iter>=0) { $info = "$last_iter: $info"; };
  return $info;
}

function output_state_for_form() {
  global $setup;
  print "<input type=hidden name=setup value=\"$setup\">\n";
}

function return_state_for_link() {
  global $setup;
  return "setup=$setup";
}

function show() {
  global $dir;
  $extra = "";
  if (preg_match("/\.(\d+)/",$_GET["show"],$match)) {
    $run = $match[1];
    if (file_exists($dir."/steps/new") || file_exists($dir."/steps/$run")) {
      $extra = "$run/";
    }
  }

  $fullname = $dir."/steps/".$extra.$_GET["show"];
  if (preg_match("/\//",$_GET["show"])) { 
    $fullname = $dir."/".$_GET["show"];
  }
  if (preg_match("/graph/",$fullname)) {
    header("Content-type: image/png");
  }
  else {
    header("Content-type: text/plain");
  }
  readfile($fullname);
  exit;
}

function set_step_status($fileName) {
  $cmd = "./progress.perl $fileName 2>/dev/null";
  #print $cmd."<p>";
  system($cmd);
}
