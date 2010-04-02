<?php

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
  return strftime("%d %b",$timestamp);
}

function load_experiment_info() {
  global $dir,$task,$user,$setup;
  global $evalset;
  global $experiment;

  $setup = $_POST[setup];
  if ($_GET[setup]) { $setup= $_GET[setup]; }
  $all_setup = file("setup");
  while (list($id,$line) = each($all_setup)) {
    $info = explode(";",$line);
    if ($info[0] == $setup) {
      $user = $info[1];
      $task = $info[2];
      $dir = rtrim($info[3]);
    }
  }

  if (file_exists($dir."/steps/new")) {
    $topd = dir($dir."/steps");
    while (false !== ($run = $topd->read())) {
      if (preg_match('/^([0-9]+)$/',$run,$match) && $run>0) {
        $d = dir($dir."/steps/$run");
        while (false !== ($entry = $d->read())) {
          process_file_entry("$dir/steps/$run/",$entry);
       }
      }
    }
  }
  else {
    $d = dir($dir."/steps");
    while (false !== ($entry = $d->read())) {
      process_file_entry($dir."/steps",$entry);
    }
  }

  reset($experiment);
  while (list($id,$info) = each($experiment)) {
    if (file_exists($dir."/steps/new")) {
      $stat = stat("$dir/steps/$id/parameter.$id");
    }
    else {
      $stat = stat("$dir/steps/parameter.$id");
    }
    $experiment[$id]->start = $stat[9];
  }
  
  reset($experiment);
  while (list($id,$info) = each($experiment)) {
    if (! file_exists($f)) {
      $f = file("$dir/evaluation/report.$id");
      foreach ($f as $line_num => $line) {
	if (preg_match('/^(.+): (.+)/',$line,$match)) {
	  $experiment[$id]->result[$match[1]] = $match[2];
	  $evalset[$match[1]]++;
	}
      }
    }
  }
  
  krsort($experiment);
  ksort($evalset);
}

function process_file_entry($dir,$entry) {
  global $experiment;
  if (preg_match('/running.(\d+)/',$entry,$match)) {
    $stat = stat($dir."/".$entry);
    $experiment[$match[1]]->end = $stat[9];
  }
  else if (preg_match('/^([A-Z\-]+)_([^\.]+)\.(\d+)$/',$entry,$match)) {
    $step = $match[1]."<BR>".$match[2];
    $run = $match[3];
    if ($run > 0 ) {
        $file = $dir.$entry;
	$stat = stat($file);
	if (file_exists($file.".STDERR")) { $stat = stat($file.".STDERR"); }
	if (file_exists($file.".STDOUT")) { $stat2 = stat($file.".STDOUT"); }
	if ($stat2[9] > $stat[9]) { $stat = $stat2; }
	$time = $stat[9];
	
	if (!$experiment[$run]->last_step_time ||
	    $time > $experiment[$run]->last_step_time) {
	  $experiment[$run]->last_step_time = $time;
	  $experiment[$run]->last_step = $step;
	}
    }
  }
}

function intro() {
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
  print "<P>To add experiment, edit /disk4/html/experiment/setup on thor";
}

function overview() {
  global $evalset;
  global $experiment,$comment;
  global $task,$user,$setup;
  global $dir;

  head("Task: $task ($user)");
  print "<a href=\"http://www.statmt.org/wiki/?n=Experiment.$setup\">Wiki Notes</a>";
  print " &nbsp; &nbsp; | &nbsp; &nbsp; <a href=\"/\">Overview of experiments</a> | <code>$dir</code><p>";
  reset($experiment);
  print "<form action=\"\" method=post>\n";
  output_state_for_form();
  print "<table border=1 cellpadding=1 cellspacing=0>
<tr><td><input type=submit name=diff value=\"compare\"></td>
    <td align=center>ID</td>
    <td align=center>start</td>
    <td align=center>end</td>\n";
  reset($evalset);
  while (list($set,$dummy) = each($evalset)) {
    print "<td align=center>$set</td>";
  }
  print "</tr>\n";
  while (list($id,$info) = each($experiment)) {
    $state = return_state_for_link();
    print "<tr><td><input type=checkbox name=run[] value=$id><a href=\"?$state&show=config.$id\">cfg</a>|<a href=\"?$state&show=parameter.$id\">par</a>|<a href=\"?$state&show=graph.$id.png\">img</a></td><td><span id=run-$setup-$id><a href='javascript:createCommentBox(\"$setup-$id\");'>[$setup-$id]</a>";
    if ($comment["$setup-$id"]) {
      print " ".$comment["$setup-$id"]->name;
    }
    print "</span></td><td align=center>".mytime($info->start,0)."</td><td align=center>";

    if (mytime($info->end,1) == "running") {
      print "<font size=-2>".$info->last_step;
      if ($info->last_step == "TUNING<BR>tune") {
        print "<BR>".tune_status($id);
      }
    }
    else if ($info->result) {
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
    if ($comment["$setup-$id"]) {
      print "currentComment[\"$setup-$id\"] = \"".$comment["$setup-$id"]->name."\";\n";
    }
  }
  print "</script>\n";
}

function output_score($id,$info) {
  global $evalset;
  reset($evalset);
  $state = return_state_for_link();

  while (list($set,$dummy) = each($evalset)) {
    $score = $info->result[$set];
    print "<td align=center>";
    $each_score = explode(" ; ",$score);
    for($i=0;$i<count($each_score);$i++) {
      if (preg_match('/([\d\(\)\.\s]+) (BLEU[\-c]*)/',$each_score[$i],$match) ||
          preg_match('/([\d\(\)\.\s]+) (IBM[\-c]*)/',$each_score[$i],$match)) {
        if ($i>0) { print "<BR>"; }
        if ($set != "avg") { print "<a href=\"?$state&show=evaluation/$set.output.$id\">"; }
        if ($set == "avg" && count($each_score)>1) { print $match[2].": "; }
        print $match[1];
        if ($set != "avg") { print "</a>"; }
      }
      else {
        print "-";
      }
    }
    print "</td>";
  }
}

function tune_status($id) {
  global $dir;
  $max_iteration = 0;
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
  $moses_ini = file($moses_ini);
  while (list($id,$line) = each($moses_ini)) {
    if (preg_match('/# BLEU ([\d\. \-\>]+)/',$line,$match)) {
      $info = $match[1];
    }
    if (preg_match('/# We were before running iteration (\d+)/',$line,$match)) {
      $last_iter = $match[1];
    }
  }
  if ($last_iter) { $info = "$last_iter: $info"; };
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

function diff() {
  global $experiment;
  $display = $_POST[run];
  sort($display);
  while (list($i,$run) = each($display)) {
    if ($i==0) {
      print "<H3>Experiment $run</H3>\n";
    }
    else {
      $diff = compute_diff($display[0],$run);
    }
    print "<table border=1 cellpadding=1 cellspacing=0><tr>";
    output_score($run,$experiment[$run]);
    print "</tr></table>";
  }
}

function compute_diff($base,$change) {
  $parameter_base = load_parameter($base);
  $parameter_change = load_parameter($change);
  print "<H3>Experiment $change</H3><TABLE>";
  while (list($parameter,$base_value) = each($parameter_base)) {
    if ($base_value != $parameter_change[$parameter]) {
      output_diff_line($parameter,$base_value,$parameter_change[$parameter]);
    }
  }
  while (list($parameter,$change_value) = each($parameter_change)) {
    if (!$parameter_base[$parameter]) {
      output_diff_line($parameter,"",$change_value);
    }
  }
  print "</TABLE>\n";
}

function output_diff_line($parameter,$base_value,$change_value) {
  print "<TR><TD BGCOLOR=yellow>$parameter</TD><TD BGCOLOR=lightgreen>$change_value</TD></TR><TR><TD>&nbsp;</TD><TD BGCOLOR=#cccccc>$base_value</TD></TR>\n";
}

function load_parameter($run) {
  global $dir;
  if (file_exists($dir."/steps/new")) {
    $file = file("$dir/steps/$run/parameter.$run");
  } 
  else {
  $file = file("$dir/steps/parameter.$run");
  }
  while (list($i,$line) = each($file)) {
    if (preg_match("/^(\S+) = (.+)$/",$line,$match)) {
      $parameter[$match[1]] = $match[2];
    }
  }
  return $parameter;
}

function load_comment() {
  global $comment;
  $file = file("comment");
  while (list($i,$line) = each($file)) {
    $line = chop($line);
    $item = explode(";",$line,4);
    $comment[$item[0]]->name = $item[1];
    $comment[$item[0]]->url = $item[2];
    $comment[$item[0]]->description = $item[3];
  }
}

function edit_comment() {
  
}

function show() {
  global $dir;
  $extra = "";
  if (file_exists($dir."/steps/new") && preg_match("/\.(\d+)/",$_GET[show],$match)) {
    $extra = "$match[1]/";
  }

  $fullname = $dir."/steps/".$extra.$_GET[show];
  if (preg_match("/\//",$_GET[show])) { 
    $fullname = $dir."/".$_GET[show];
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

function head($title) {
  print "<HTML><HEAD><TITLE>$title</TITLE></HEAD>\n<BODY><H2>$title</H2>\n";
}

if ($_POST[setup] || $_GET[setup]) {
  load_experiment_info();
  load_comment();
  if ($_GET [show]) { show(); }
  else if ($_POST[diff]) { diff(); }
  else { overview(); }
}
else {
  intro();
}

?>
<script language="javascript" type="text/javascript">
<!--
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

//-->
</script>
</BODY></HTML>
