<?php

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
  print "<P>To add experiment, edit /disk4/html/experiment/setup on thor";
}

function overview() {
  global $evalset;
  global $experiment,$comment;
  global $task,$user,$setup;
  global $dir;
  global $has_analysis;

  head("Task: $task ($user)");
  print "<a href=\"http://www.statmt.org/wiki/?n=Experiment.$setup\">Wiki Notes</a>";
  print " &nbsp; &nbsp; | &nbsp; &nbsp; <a href=\"/\">Overview of experiments</a> &nbsp; &nbsp; | &nbsp; &nbsp; <code>$dir</code><p>";
  reset($experiment);

  print "<form action=\"\" method=get>\n";
  output_state_for_form();
  while (list($id,$info) = each($experiment)) {
    reset($evalset);
    while (list($set,$dummy) = each($evalset)) {
      $analysis = "$dir/evaluation/$set.analysis.$id";
      if (file_exists($analysis)) {
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
    if ($has_analysis[$set]) {
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
  return strftime("%d %b",$timestamp);
}

function output_score($id,$info) {
  global $evalset;
  global $has_analysis;
  global $setup;
  reset($evalset);
  $state = return_state_for_link();

  while (list($set,$dummy) = each($evalset)) {
    $score = $info->result[$set];
    print "<td align=center>";

    // print "<table><tr><td>";

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
    if ($has_analysis[$set]) {
      print "<td align=center>";
      global $dir;
      $analysis = "$dir/evaluation/$set.analysis.$id";
      if (file_exists($analysis)) {
        print "<a href=\"?analysis=show&setup=$setup&set=$set&id=$id\">analysis</a><br><input type=checkbox name=analysis-$id-$set value=1>";
      }
      print "</td>";
    }
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
