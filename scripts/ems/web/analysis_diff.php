<?php 

function diff_analysis() {
  global $task,$user,$setup,$id,$id2,$set;
  global $comment,$dir;

  head("Comparative Analysis: $task ($user), Set $set");

  $c = $comment["$setup-$id"]->name;
  $c2 = $comment["$setup-$id2"]->name;
  if (substr($c2,0,strlen($id)+1) == $id."+") {
      print "Run $id2 vs $id (".substr($c2,strlen($id)).")";
  }
  else {
      print "Run $id2 ($c2) vs $id ($c)";
  }
  print "</h4>";
  
?><script language="javascript" src="/javascripts/prototype.js"></script>
<script language="javascript" src="/javascripts/scriptaculous.js"></script>
<script>
function diff(field,sort,count) {
  var url = '?analysis_diff=' + field + '_diff'
      + '&setup=<?php print $setup ?>'
      + '&id=<?php print $id ?>'
      + '&id2=<?php print $id2 ?>'
      + '&set=<?php print $set ?>'
      + '&sort=' + sort
      + '&count=' + count;
  new Ajax.Updater(field, url, { method: 'get' });
}
function ngram_diff(type,order,count,sort,smooth) {
  var url = '?analysis_diff=ngram_' + type + '_diff'
      + '&setup=<?php print $setup ?>'
      + '&id=<?php print $id ?>'
      + '&id2=<?php print $id2 ?>'
      + '&set=<?php print $set ?>'
      + '&order=' + order
      + '&smooth=' + smooth
      + '&sort=' + sort
      + '&count=' + count;
  var field = (type == "precision" ? "nGramPrecision" : "nGramRecall") + order;
  new Ajax.Updater(field, url, { method: 'get' });
}
function generic_show_diff(field,parameters) {
  var url = '?analysis=' + field + '_show'
      + '&setup=<?php print $setup ?>'
      + '&id=<?php print $id ?>'
      + '&id2=<?php print $id2 ?>'
      + '&set=<?php print $set ?>'
      + '&' + parameters;
  new Ajax.Updater(field, url, { method: 'get', evalScripts: true });
}
</script>
</head>
<body>
<div id="nGramSummary"><?php ngram_summary_diff() ?></div>
<div id="PrecisionByCoverageDiff"></div>
<div id="PrecisionRecallDetailsDiff"></div>
<div id="bleu">(loading...)</div>
<script>
diff('bleu','',5);
</script>
</body></html>
<?php
}

function precision_by_coverage_diff() {
  global $experiment,$evalset,$dir,$set,$id,$id2;
  $img_width = 1000;

  print "<h3>Precision by Coverage</h3>";
  print "The graphs display what ratio of words of a specific type are translated correctly (yellow), and what ratio is deleted (blue).";
  print " The extend of the boxes is scaled on the x-axis by the number of tokens of the displayed type.";
  // load data
  $data = file(get_current_analysis_filename2("precision","precision-by-corpus-coverage"));
  $total = 0;
  $log_info = array();
  for($i=0;$i<count($data);$i++) {
    $item = split("\t",$data[$i]);
    $info[$item[0]]["precision"] = $item[1];
    $info[$item[0]]["delete"] = $item[2];
    $info[$item[0]]["length"] = $item[3];
    $info[$item[0]]["total"] = $item[4];
    $total += $item[4];
    $log_count = -1;
    if ($item[0]>0) {
	$log_count = (int) (log($item[0])/log(2));
    }
    if (!array_key_exists($log_count,$log_info)) {
	$log_info[$log_count]["precision"] = 0;
	$log_info[$log_count]["delete"] = 0;
	$log_info[$log_count]["length"] = 0;
	$log_info[$log_count]["total"] = 0;
    }
    $log_info[$log_count]["precision"] += $item[1];
    $log_info[$log_count]["delete"] += $item[2];
    $log_info[$log_count]["length"] += $item[3];
    $log_info[$log_count]["total"] += $item[4];    
  }
  $log_info_new = $log_info;  

  // load base data
  $data = file(get_current_analysis_filename("precision","precision-by-corpus-coverage"));
  for($i=0;$i<count($data);$i++) {
    $item = split("\t",$data[$i]);
    $info[$item[0]]["precision"] -= $item[1];
    $info[$item[0]]["delete"] -= $item[2];
    $info[$item[0]]["length"] -= $item[3];
    $log_count = -1;
    if ($item[0]>0) {
	$log_count = (int) (log($item[0])/log(2));
    }
    $log_info[$log_count]["precision"] -= $item[1];
    $log_info[$log_count]["delete"] -= $item[2];
    $log_info[$log_count]["length"] -= $item[3];
  }

  print "<h4>By log<sub>2</sub>-count in the training corpus</h4>";
  precision_by_coverage_diff_graph("byCoverage",$log_info,$log_info_new,$total,$img_width,SORT_NUMERIC);

  // load factored data
  $d = dir("$dir/evaluation/$set.analysis.".get_precision_analysis_version($dir,$set,$id));
  while (false !== ($file = $d->read())) {
    if (preg_match('/precision-by-corpus-coverage.(.+)$/',$file, $match) &&
	file_exists(get_current_analysis_filename2("precision","precision-by-corpus-coverage.$match[1]"))) {
      precision_by_coverage_diff_factored($img_width,$total,$file,$match[1]);
    }
  }
}

function precision_by_coverage_diff_factored($img_width,$total,$file,$factor_id) {
  global $dir,$set,$id,$id2;
  $data = file(get_current_analysis_filename2("precision",$file));
  for($i=0;$i<count($data);$i++) {
    $item = split("\t",$data[$i]);
    $factor = $item[0];
    $count = $item[1];
    $info_factored[$factor][$count]["precision"] = $item[2];
    $info_factored[$factor][$count]["delete"] = $item[3];
    $info_factored[$factor][$count]["length"] = $item[4];
    $info_factored[$factor][$count]["total"] = $item[5];
    $info_factored_sum[$factor]["precision"] += $item[2];
    $info_factored_sum[$factor]["delete"] += $item[3];
    $info_factored_sum[$factor]["length"] += $item[4];
    $info_factored_sum[$factor]["total"] += $item[5];
    $total_factored[$factor] += $item[5];
    $log_count = -1;
    if ($count>0) {
	$log_count = (int) (log($count)/log(2));
    }
    $log_info_factored[$factor][$log_count]["precision"] += $item[2];
    $log_info_factored[$factor][$log_count]["delete"] += $item[3];
    $log_info_factored[$factor][$log_count]["length"] += $item[4];
    $log_info_factored[$factor][$log_count]["total"] += $item[5];    
  }
  $info_factored_new = $info_factored;
  $info_factored_sum_new = $info_factored_sum;
  $log_info_factored_new = $log_info_factored;

  // baseline data
  $data = file(get_current_analysis_filename("precision",$file));
  for($i=0;$i<count($data);$i++) {
    $item = split("\t",$data[$i]);
    $factor = $item[0];
    $count = $item[1];
    $info_factored[$factor][$count]["precision"] -= $item[2];
    $info_factored[$factor][$count]["delete"] -= $item[3];
    $info_factored[$factor][$count]["length"] -= $item[4];
    $info_factored_sum[$factor]["precision"] -= $item[2];
    $info_factored_sum[$factor]["delete"] -= $item[3];
    $info_factored_sum[$factor]["length"] -= $item[4];
    $log_count = -1;
    if ($count>0) {
	$log_count = (int) (log($count)/log(2));
    }
    $log_info_factored[$factor][$log_count]["precision"] -= $item[2];
    $log_info_factored[$factor][$log_count]["delete"] -= $item[3];
    $log_info_factored[$factor][$log_count]["length"] -= $item[4];
  }
  print "<h4>By factor ".factor_name("input",$factor_id)."</h4>";
  precision_by_coverage_diff_graph("byFactor",$info_factored_sum,$info_factored_sum_new,$total,$img_width,SORT_STRING);

  print "<h4>For each factor, by log<sub>2</sub>-count in the corpus</h4>";
  foreach ($log_info_factored as $factor => $info) {
    if ($total_factored[$factor]/$total > 0.01) {
      print "<table style=\"display:inline;\"><tr><td align=center><font size=-2><b>$factor</b></font>";
      precision_by_coverage_diff_graph("byCoverageFactor$factor",$info,$log_info_factored_new[$factor],$total,10+2*$img_width*$total_factored[$factor]/$total,SORT_NUMERIC);
      print "</td></tr></table>";
    }
  }
}

function precision_by_word_diff($type) {
  global $dir,$set,$id,$id2;
  $byCoverage = -2;
  $byFactor = "false";
  if ($type == "byCoverage") {
      $byCoverage = (int) $_GET["type"];
  }
  else if ($type == "byFactor") {
      $byFactor = $_GET["type"];
  }
  else if (preg_match("/byCoverageFactor(.+)/",$type,$match)) {
      $byCoverage = (int) $_GET["type"];
      $byFactor = $match[1];
  }

  $data = file(get_current_analysis_filename2("precision","precision-by-input-word"));
  $total = 0;
  $info = array();
  for($i=0;$i<count($data);$i++) {
    $line = rtrim($data[$i]);
    $item = split("\t",$line);
    $total += $item[3];

    //# filter for count
    $count = $item[4];
    $log_count = -1;
    if ($count>0) {
      $log_count = (int) (log($count)/log(2));
    }
    if ($byCoverage != -2 && $byCoverage != $log_count) {
      continue;
    }
   
    //# filter for factor
    $word = $item[5];
    if ($byFactor != "false" && $byFactor != $item[6]) {
      continue;
    }
    if (!array_key_exists($word,$info)) {
      $info[$word]["precision"] = 0;
      $info[$word]["delete"] = 0;
      $info[$word]["length"] = 0;
      $info[$word]["total"] = 0;
    }
    $info[$word]["precision"] += $item[0];
    $info[$word]["delete"] += $item[1];
    $info[$word]["length"] += $item[2];
    $info[$word]["total"] += $item[3];
  }
  $info_new = $info;

  $data = file(get_current_analysis_filename("precision","precision-by-input-word"));
  for($i=0;$i<count($data);$i++) {
    $line = rtrim($data[$i]);
    $item = split("\t",$line);

    //# filter for count
    $count = $item[4];
    $log_count = -1;
    if ($count>0) {
	$log_count = (int) (log($count)/log(2));
    }
    if ($byCoverage != -2 && $byCoverage != $log_count) {
	continue;
    }
   
    //# filter for factor
    $word = $item[5];
    if ($byFactor != "false" && $byFactor != $item[6]) {
	continue;
    }
    if (!array_key_exists($word,$info)) {
      $info[$word]["precision"] = 0;
      $info[$word]["delete"] = 0;
      $info[$word]["length"] = 0;
      $info_new[$word]["length"] = 0;
      $info_new[$word]["delete"] = 0;
      $info_new[$word]["precision"] = 0;
      $info_new[$word]["total"] = 0;
      $info[$word]["total"] = -$item[3];
    }
    $info[$word]["precision"] -= $item[0];
    $info[$word]["delete"] -= $item[1];
    $info[$word]["length"] -= $item[2];
  }

  print "<table border=1><tr><td align=center>&nbsp;</td><td align=center colspan=3>Precision</td><td align=center colspan=2>Precision Impact</td><td align=center colspan=3>Delete</td><td align=center colspan=2>Delete Impact</td><td align=center>Length</td></tr>\n";
  foreach ($info as $word => $wordinfo) {
      print "<tr><td align=center>$word</td>";
      printf("<td align=right>%.1f%s</td><td align=right>%+.1f%s</td><td align=right><font size=-1>%+.1f/%d</font></td>",$info_new[$word]["precision"]/$wordinfo["total"]*100,"%",$wordinfo["precision"]/$wordinfo["total"]*100,"%",$wordinfo["precision"],$wordinfo["total"]);
      printf("<td align=right>%+.2f%s</td><td align=right><font size=-1>%+.1f/%d</font></td>",$wordinfo["precision"]/$total*100,"%",$wordinfo["precision"],$total);
      printf("<td align=right>%.1f%s</td><td align=right>%+.1f%s</td><td align=right><font size=-1>%+d/%d</font></td>",$info_new[$word]["delete"]/$wordinfo["total"]*100,"%",$wordinfo["delete"]/$wordinfo["total"]*100,"%",$wordinfo["delete"],$wordinfo["total"]);
      printf("<td align=right>%+.2f%s</td><td align=right><font size=-1>%+d/%d</font></td>",$wordinfo["delete"]/$total*100,"%",$wordinfo["delete"],$total);
      printf("<td align=right>%+.3f</td>",$wordinfo["length"]/$wordinfo["total"]);
      print "</tr>";
  }
  print "</table>\n";
}


function precision_by_coverage_diff_graph($name,$log_info,$log_info_new,$total,$img_width,$sort_type) {
  $keys = array_keys($log_info);
  sort($keys,$sort_type);
  
  print "<div id=\"Toggle$name\" onClick=\"document.getElementById('Table$name').style.display = 'none'; this.style.display = 'none';\" style=\"display:none;\"><font size=-2>(hide table)</font></div>\n";
  print "<div id=\"Table$name\" style=\"display:none;\">\n";
  print "<table border=1><tr><td align=center>&nbsp;</td><td align=center colspan=3>Precision</td><td align=center colspan=2>Precision Impact</td><td align=center colspan=3>Delete</td><td align=center colspan=2>Delete Impact</td><td align=center>Length</td></tr>\n";  
  foreach ($keys as $i) {
    if (array_key_exists($i,$log_info)) {
      print "<tr><td align=center>$i</td>";
      printf("<td align=right>%.1f%s</td><td align=right>%.1f%s</td><td align=right><font size=-1>%+.1f/%d</font></td>",$log_info_new[$i]["precision"]/$log_info[$i]["total"]*100,"%",$log_info[$i]["precision"]/$log_info[$i]["total"]*100,"%",$log_info[$i]["precision"],$log_info[$i]["total"]);
      printf("<td align=right>%+.2f%s</td><td align=right><font size=-1>%+.1f/%d</font></td>",$log_info[$i]["precision"]/$total*100,"%",$log_info[$i]["precision"],$total);
      printf("<td align=right>%.1f%s</td><td align=right>%+.1f%s</td><td align=right><font size=-1>%+d/%d</font></td>",$log_info_new[$i]["delete"]/$log_info[$i]["total"]*100,"%",$log_info[$i]["delete"]/$log_info[$i]["total"]*100,"%",$log_info[$i]["delete"],$log_info[$i]["total"]);
      printf("<td align=right>%+.2f%s</td><td align=right><font size=-1>%+d/%d</font></td>",$log_info[$i]["delete"]/$total*100,"%",$log_info[$i]["delete"],$total);
      printf("<td align=right>%+.3f</td>",$log_info[$i]["length"]/$log_info[$i]["total"]);
      print "<td><A HREF=\"javascript:generic_show_diff('PrecisionByWordDiff$name','type=$i')\">&#x24BE;</A></td>";
      print "</tr>";
   }
  }
  print "</table><div id=\"PrecisionByWordDiff$name\"></div></div>";

  print "<div id=\"Graph$name\" onClick=\"document.getElementById('Table$name').style.display = 'block'; document.getElementById('Toggle$name').style.display = 'block';\"><canvas id=\"$name\" width=$img_width height=300></canvas></div>";
  print "<script language=\"javascript\">
var canvas = document.getElementById(\"$name\");
var ctx = canvas.getContext(\"2d\");
ctx.lineWidth = 0.5;
ctx.font = '9px serif';
";
  for($line=-1;$line<=0.8;$line+=.2) {
      $height = 90-$line/2*180;
      print "ctx.moveTo(20, $height);\n";
      print "ctx.lineTo($img_width, $height);\n";
      print "ctx.fillText(\"".sprintf("%d",10 * $line * 1.001)."\%\", 0, $height+4);";
  }
  for($line=-0.4;$line<=0.4;$line+=.2) {
      $height = 250+$line/2*180;
      print "ctx.moveTo(20, $height);\n";
      print "ctx.lineTo($img_width, $height);\n";
      if ($line != 0) {
	  print "ctx.fillText(\"".sprintf("%d",10 * $line * 1.001)."\%\", 0, $height+4);";
      }
  }
  print "ctx.strokeStyle = \"rgb(100,100,100)\"; ctx.stroke();\n";

  $total = 0;
  foreach ($keys as $i) {
    $total += $log_info[$i]["total"];
  }
  $total_so_far = 0;
  foreach ($keys as $i) {
    $prec_ratio = $log_info[$i]["precision"]/$log_info[$i]["total"];
    $x = (int)(20+($img_width-20) * $total_so_far / $total);
    $y = (int)(90-($prec_ratio*180*5));
    $width = (int)($img_width * $log_info[$i]["total"]/$total);
    $height = (int)($prec_ratio*180*5);
    print "ctx.fillStyle = \"rgb(200,200,0)\";";
    print "ctx.fillRect ($x, $y, $width, $height);";

    $del_ratio = $log_info[$i]["delete"]/$log_info[$i]["total"];
    $height = (int)($del_ratio*180*5);
    print "ctx.fillStyle = \"rgb(100,100,255)\";";
    print "ctx.fillRect ($x, 250, $width, $height);";

    $total_so_far += $log_info[$i]["total"];
   
    if ($width>3) {
      print "ctx.fillStyle = \"rgb(0,0,0)\";";
      // print "ctx.rotate(-1.5707);";
      print "ctx.fillText(\"$i\", $x+$width/2-3, 190);";
      //print "ctx.rotate(1.5707);";
    }
  }
  print "</script>";
}


// stats on precision and recall
function precision_recall_details_diff() {
?>
<table width=100%>
  <tr>
    <td width=25% valign=top><div id="nGramPrecision1">(loading...)</div></td>
    <td width=25% valign=top><div id="nGramPrecision2">(loading...)</div></td>
    <td width=25% valign=top><div id="nGramPrecision3">(loading...)</div></td>
    <td width=25% valign=top><div id="nGramPrecision4">(loading...)</div></td>
  </tr><tr>
    <td width=25% valign=top><div id="nGramRecall1">(loading...)</div></td>
    <td width=25% valign=top><div id="nGramRecall2">(loading...)</div></td>
    <td width=25% valign=top><div id="nGramRecall3">(loading...)</div></td>
    <td width=25% valign=top><div id="nGramRecall4">(loading...)</div></td>
</tr></table>
<script language="javascript">
ngram_diff('precision',1,5,'',0);
ngram_diff('precision',2,5,'',0);
ngram_diff('precision',3,5,'',0);
ngram_diff('precision',4,5,'',0);
ngram_diff('recall',1,5,'',0);
ngram_diff('recall',2,5,'',0);
ngram_diff('recall',3,5,'',0);
ngram_diff('recall',4,5,'',0);
</script>
<?php
}

function ngram_summary_diff() {
  global $experiment,$evalset,$dir,$set,$id,$id2;

  // load data
  for($idx=0;$idx<2;$idx++) {
    $data = file(get_analysis_filename($dir,$set,$idx?$id2:$id,"basic","summary"));
    for($i=0;$i<count($data);$i++) {
      $item = split(": ",$data[$i]);
      $info[$idx][$item[0]] = $item[1];
    }
  }

  print "<table cellspacing=5 width=100%><tr><td valign=top align=center bgcolor=#eeeeee>";
  print "<b>Precision of Output</b><br>";
  //foreach (array("precision","recall") as $type) {
  $type = "precision";
  print "<table><tr><td>$type</td><td>1-gram</td><td>2-gram</td><td>3-gram</td><td>4-gram</td></tr>\n";
  printf("<tr><td>correct</td><td>%d (%+d)</td><td>%d (%+d)</td><td>%d (%+d)</td><td>%d (%+d)</td></tr>\n",
	 $info[1]["$type-1-correct"],$info[1]["$type-1-correct"]-$info[0]["$type-1-correct"],
	 $info[1]["$type-2-correct"],$info[1]["$type-2-correct"]-$info[0]["$type-2-correct"],
	 $info[1]["$type-3-correct"],$info[1]["$type-3-correct"]-$info[0]["$type-3-correct"],
	 $info[1]["$type-4-correct"],$info[1]["$type-4-correct"]-$info[0]["$type-4-correct"]);
  printf("<tr><td>&nbsp;</td><td>%.1f%s (%+.1f%s)</td><td>%.1f%s (%+.1f%s)</td><td>%.1f%s (%+.1f%s)</td><td>%.1f%s (%+.1f%s)</td></tr>\n",
	 $info[1]["$type-1-correct"]/$info[1]["$type-1-total"]*100,'%',$info[1]["$type-1-correct"]/$info[1]["$type-1-total"]*100-$info[0]["$type-1-correct"]/$info[0]["$type-1-total"]*100,'%',
	 $info[1]["$type-2-correct"]/$info[1]["$type-2-total"]*100,'%',$info[1]["$type-2-correct"]/$info[1]["$type-2-total"]*100-$info[0]["$type-2-correct"]/$info[0]["$type-2-total"]*100,'%',
	 $info[1]["$type-3-correct"]/$info[1]["$type-3-total"]*100,'%',$info[1]["$type-3-correct"]/$info[1]["$type-3-total"]*100-$info[0]["$type-3-correct"]/$info[0]["$type-3-total"]*100,'%',
	 $info[1]["$type-4-correct"]/$info[1]["$type-4-total"]*100,'%',$info[1]["$type-4-correct"]/$info[1]["$type-4-total"]*100-$info[0]["$type-4-correct"]/$info[0]["$type-4-total"]*100,'%');
  printf("<tr><td>wrong</td><td>%d (%+d)</td><td>%d (%+d)</td><td>%d (%+d)</td><td>%d (%+d)</td></tr>\n",
	 $info[1]["$type-1-total"]-$info[1]["$type-1-correct"],($info[1]["$type-1-total"]-$info[1]["$type-1-correct"])-($info[0]["$type-1-total"]-$info[0]["$type-1-correct"]),
	 $info[1]["$type-2-total"]-$info[1]["$type-2-correct"],($info[1]["$type-2-total"]-$info[1]["$type-2-correct"])-($info[0]["$type-2-total"]-$info[0]["$type-2-correct"]),
	 $info[1]["$type-3-total"]-$info[1]["$type-3-correct"],($info[1]["$type-3-total"]-$info[1]["$type-3-correct"])-($info[0]["$type-3-total"]-$info[0]["$type-3-correct"]),
	 $info[1]["$type-4-total"]-$info[1]["$type-4-correct"],($info[1]["$type-4-total"]-$info[1]["$type-4-correct"])-($info[0]["$type-4-total"]-$info[0]["$type-4-correct"]));
  print "</table>";
  //}

  print "<A HREF=\"javascript:generic_show_diff('PrecisionRecallDetailsDiff','')\">details</A> ";
  if (file_exists(get_current_analysis_filename("precision","precision-by-corpus-coverage")) &&
      file_exists(get_current_analysis_filename2("precision","precision-by-corpus-coverage"))) {
    print "| <A HREF=\"javascript:generic_show_diff('PrecisionByCoverageDiff','')\">precision of input by coverage</A> ";
  }

  print "</td><td valign=top align=center bgcolor=#eeeeee>";
  print "<b>Metrics</b><br>\n";

  for($idx=0;$idx<2;$idx++) {
    $each_score = explode(" ; ",$experiment[$idx?$id2:$id]->result[$set]);
    for($i=0;$i<count($each_score);$i++) {
      if (preg_match('/([\d\(\)\.\s]+) (BLEU[\-c]*)/',$each_score[$i],$match) ||
          preg_match('/([\d\(\)\.\s]+) (IBM[\-c]*)/',$each_score[$i],$match)) {
	  $score[$match[2]][$idx] = $match[1];
      }
    }
  }
  $header = ""; $score_line = ""; $diff_line = "";
  foreach ($score as $name => $value) {
    $header .= "<td>$name</td>";
    $score_line .= "<td>".$score[$name][1]."</td>";
    $diff_line .= sprintf("<td>%+.2f</td>",$score[$name][1]-$score[$name][0]);
  }
  print "<table border=1><tr>".$header."</tr><tr>".$score_line."</tr><tr>".$diff_line."</tr></table>";
  printf("length-diff<br>%d (%+d)",$info[1]["precision-1-total"]-$info[1]["recall-1-total"],$info[1]["precision-1-total"]-$info[0]["precision-1-total"]);

  print "</td><tr><table>";
}

function bleu_diff() {
  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }

  print "<b>annotated sentences</b><br>";
  print "<font size=-1>sorted by ";

  if ($_GET['sort'] == "order" || $_GET['sort'] == "") {
    print "order ";
  }
  else {
    print "<A HREF=\"javascript:diff('bleu','order',$count)\">order</A> ";
  }

  if ($_GET['sort'] == "better") {
    print "order ";
  }
  else {
    print "<A HREF=\"javascript:diff('bleu','better',$count)\">better</A> ";
  }

  if ($_GET['sort'] == "worse") {
    print "order ";
  }
  else {
    print "<A HREF=\"javascript:diff('bleu','worse',$count)\">worse</A> ";
  }

  print "display <A HREF=\"\">fullscreen</A> ";

  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }
  print "showing $count ";
  print "<A HREF=\"javascript:diff('bleu','" . $_GET['sort'] . "',5+$count)\">more</A> ";
  if ($count > 5) {
    print "<A HREF=\"javascript:diff('bleu','" . $_GET['sort'] . "',$count-5)\">less</A> ";
  }
  print "<A HREF=\"javascript:diff('bleu','" . $_GET['sort'] . "',9999)\">all</A> ";

  print "</font><BR>\n";

  bleu_diff_annotation();
}

function bleu_diff_annotation() {
  global $set,$id,$id2,$dir;

  // load data
  for($idx=0;$idx<2;$idx++) {
    $data = file(get_analysis_filename($dir,$set,$idx?$id2:$id,"basic","bleu-annotation"));
    for($i=0;$i<count($data);$i++) {
      $item = split("\t",$data[$i]);
      $annotation[$item[1]]["bleu$idx"] = $item[0]; 
      $annotation[$item[1]]["system$idx"] = $item[2]; 
      $annotation[$item[1]]["reference"] = $item[3]; 
      $annotation[$item[1]]["id"] = $item[1];
    }
  }
  $data = array();

  $identical=0; $same=0; $better=0; $worse=0;
  for($i=0;$i<count($annotation);$i++) {
    if ($annotation[$i]["system1"] == $annotation[$i]["system0"]) {
      $identical++;
    }
    else if ($annotation[$i]["bleu1"] == $annotation[$i]["bleu0"]) {
      $same++;
    }
    else if ($annotation[$i]["bleu1"] > $annotation[$i]["bleu0"]) {
      $better++;
    }
    else {
      $worse++;
    }
  }

  print "<table><tr><td>identical</td><td>same</td><td>better</td><td>worse</td></tr>\n";
  printf("<tr><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>\n", $identical, $same, $better, $worse);
  printf("<tr><td>%d%s</td><td>%d%s</td><td>%d%s</td><td>%d%s</td></tr></table>\n", $identical*100/count($annotation)+.5, '%', $same*100/count($annotation)+.5, '%', $better*100/count($annotation)+.5, '%', $worse*100/count($annotation)+.5, '%');

//  print "identical: $identical (%d), same: $same, better: $better, worse: $worse<br>\n";

  // sort
  global $sort;
  $sort = $_GET['sort'];
  if ($sort == '') {
    $sort = "order";
  }
  function cmp($a, $b) {
    global $sort;
    if ($sort == "worse") {
      $a_idx = $a["bleu1"]-$a["bleu0"];
      $b_idx = $b["bleu1"]-$b["bleu0"];
    }
    else if ($sort == "better") {
      $a_idx = -$a["bleu1"]+$a["bleu0"];
      $b_idx = -$b["bleu1"]+$b["bleu0"];
    }

    if ($a_idx == $b_idx) {
        return 0;
    }
    return ($a_idx < $b_idx) ? -1 : 1;
  }

  if ($sort != 'order') {
    usort($annotation, 'cmp');
  }

  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }

  // display
  for($i=0;$i<$count && $i<count($annotation);$i++) {
     $line = $annotation[$i]; 

     $word_with_score1 = split(" ",$line["system1"]);
     $word_with_score0 = split(" ",$line["system0"]);
     $word1 = split(" ",preg_replace("/\|\d/","",$line["system1"]));
     $word0 = split(" ",preg_replace("/\|\d/","",$line["system0"]));
     $matched_with_score = string_edit_distance($word_with_score0,$word_with_score1);
     $matched = string_edit_distance($word0,$word1);

     print "<font size=-2>[".$id2."-".$line["id"].":".$line["bleu1"]."]</font> ";
     $matched1 = preg_replace('/D/',"",$matched);
     $matched_with_score1 = preg_replace('/D/',"",$matched_with_score);
     bleu_line_diff( $word_with_score1, $matched1, $matched_with_score1 );

     print "<font size=-2>[".$id."-".$line["id"].":".$line["bleu0"]."]</font> ";     
     $matched0 = preg_replace('/I/',"",$matched);
     $matched_with_score0 = preg_replace('/I/',"",$matched_with_score);
     bleu_line_diff( $word_with_score0, $matched0, $matched_with_score0 );

     print "<font size=-2>[ref]</font> ".$line["reference"]."<hr>";
  }
}

function bleu_line_diff( $word,$matched,$matched_with_score ) {
  $color = array("#FFC0C0","#FFC0FF","#C0C0FF","#C0FFFF","#C0FFC0");
  $lcolor = array("#FFF0F0","#FFF0FF","#F0F0FF","#F0FFFF","#F0FFF0");
  for($j=0;$j<count($word);$j++) {
    list($surface,$correct) = split("\|", $word[$j]);
    if (substr($matched_with_score,$j,1) == "M") {
      $style = "background-color: $lcolor[$correct];";
    }
    else {
      $style = "background-color: $color[$correct];";
    }
    if (substr($matched,$j,1) == "M") {
      $style .= "color: #808080;";
    }
    print "<span style=\"$style\">$surface</span> ";
  }
  print "<br>";
}

function ngram_diff($type) {
  global $set,$id,$id2,$dir;

  ini_set('memory_limit',1e9); // 1G for big files

  // load data
  $order = $_GET['order'];
  
  for($idx=0;$idx<2;$idx++) {
    $data = file(get_analysis_filename($dir,$set,$idx?$id2:$id,"basic","n-gram-$type.$order"));
    for($i=0;$i<count($data);$i++) {
      $item = split("\t",$data[$i]);
      $ngram_hash[$item[2]]["total$idx"] = $item[0]; 
      $ngram_hash[$item[2]]["correct$idx"] = $item[1];
    }  
    unset($data);
  }

  // sort option
  $sort = $_GET['sort'];
  $smooth = $_GET['smooth'];
  if ($sort == '') {
    $sort = 'ratio_worse';
    $smooth = 1;
  }
  
  error_reporting(E_ERROR); // otherwise undefined counts trigger notices

  // sort index
  foreach ($ngram_hash as $n => $value) {
    $item = $value;
//    $item["correct0"] += 0;
//    $item["correct1"] += 0;
//    $item["total0"] += 0;
//    $item["total1"] += 0;
    $item["ngram"] = $n;

    if ($sort == "abs_worse") {
      $item["index"] = (2*$item["correct1"] - $item["total1"])
        - (2*$item["correct0"] - $item["total0"]);
    }
    else if ($sort == "abs_better") {
      $item["index"] = - (2*$item["correct1"] - $item["total1"])
        + (2*$item["correct0"] - $item["total0"]);
    }
    else if ($sort == "ratio_worse") {
      $item["index"] = 
          ($item["correct1"] + $smooth) / ($item["total1"] + $smooth)
        - ($item["correct0"] + $smooth) / ($item["total0"] + $smooth);
    }
    else if ($sort == "ratio_better") {
      $item["index"] = 
        - ($item["correct1"] + $smooth) / ($item["total1"] + $smooth)
        + ($item["correct0"] + $smooth) / ($item["total0"] + $smooth);
    }
    $ngram[] = $item;
    unset($ngram_hash[$n]);
  }
  unset($ngram_hash);

  // sort
  function cmp($a, $b) {
    if ($a["index"] == $b["index"]) {
        return 0;
    }
    return ($a["index"] < $b["index"]) ? -1 : 1;
  }

  usort($ngram, 'cmp');

  // display
  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }

  print "<B>$order-gram $type</B><br><font size=-1>sorted by<br>";
  if ($sort == "ratio_worse") {
    print "ratio worse ";
    print "smooth-$smooth ";
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count,'ratio_worse',$smooth+1)\">+</A> ";
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count,'ratio_worse',$smooth-1)\">-</A>,";
  }
  else {
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count,'ratio_worse',1)\">ratio worse</A>, ";
  }
  if ($sort == "abs_worse") {
    print "absolute worse, ";
  }
  else {
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count,'abs_worse',0)\">absolute worse</A>, ";
  }

  print "<br>";
  if ($sort == "ratio_better") {
    print "ratio better ";
    print "smooth-$smooth ";
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count,'ratio_better',$smooth+1)\">+</A> ";
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count,'ratio_better',$smooth-1)\">-</A>,";
  }
  else {
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count,'ratio_better',1)\">ratio better</A>, ";
  }
  if ($sort == "abs_better") {
    print "absolute better, ";
  }
  else {
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count,'abs_better',0)\">absolute better</A>, ";
  }

  print "<br>showing $count ";
  if ($count < 9999) {
    print "<A HREF=\"javascript:ngram_diff('$type',$order,$count+5,'$sort',$smooth)\">more</A> ";
    if ($count > 5) {
      print "<A HREF=\"javascript:ngram_diff('$type',$order,$count-5,'$sort',$smooth)\">less</A> ";
    }
    print "<A HREF=\"javascript:ngram_diff('$type',$order,9999,'$sort',$smooth)\">all</A> ";
  }
  else {
    print "<A HREF=\"javascript:ngram_diff('$type',$order,5,'$sort',$smooth)\">top5</A> ";
  }

  print "<br>\n";

  print "<table width=100%>\n";
  print "<tr><td>$order-gram</td>";
  if ($type == 'recall') {
    print "<td>&Delta;</td><td>ok</td><td>x</td></tr>\n";
  }
  else {
    print "<td align=right>&Delta;</td><td>ok</td><td align=right>&Delta;</td><td>x</td></tr>\n";
  }
  for($i=0;$i<$count && $i<count($ngram);$i++) {
    $line = $ngram[$i];
    print "<tr><td>".$line["ngram"]."</td>";
    $ok = $line["correct1"];
    $ok_diff = $ok - $line["correct0"];
    $wrong = $line["total1"] - $line["correct1"];
    $wrong_diff = $wrong - ($line["total0"]-$line["correct0"]);
    if ($type == 'recall') {
      printf("<td>%+d</td><td>%d</td><td>%d</td></tr>", $ok_diff,$ok,$wrong);
    }
    else {
      printf("<td align=right>%+d</td><td>(%d)</td><td align=right>%+d</td><td>(%d)</td></tr>", $ok_diff,$ok,$wrong_diff,$wrong);
    } 
  }
  print "</table>\n";
}

function string_edit_distance($a,$b) {
  $cost = array( array( 0 ) );
  $back = array( array( "" ) );

  // init boundaries
  for($i=0;$i<count($a);$i++) {
    $cost[$i+1][0] = $i+1;
  }
  for($j=0;$j<count($b);$j++) {
    $cost[0][$j+1] = $j+1;
  }

  // exhaustive sed
  for($i=1;$i<=count($a);$i++) {
    for($j=1;$j<=count($b);$j++) {
      $match_cost = ($a[$i-1] == $b[$j-1]) ? 0 : 1;
      $c = $match_cost + $cost[$i-1][$j-1];
      $p = $match_cost ? "S" : "M";
      if ($cost[$i-1][$j]+1 < $c) {
	$c = $cost[$i-1][$j]+1;
	$p = "D";
      }
      if ($cost[$i][$j-1]+1 < $c) {
	$c = $cost[$i][$j-1]+1;
	$p = "I";
      }
      $cost[$i][$j] = $c;
      $back[$i][$j] = $p;
    }
  }

  // retrieve path
  $i=count($a);
  $j=count($b);
  $path = "";
  while($i>0 || $j>0) {
    if ($back[$i][$j] == "M" || $back[$i][$j] == "S") {
      $path = $back[$i][$j] . $path;
      $i--; $j--;
    }
    else if($i==0 || $back[$i][$j] == "I") {
      $path = "I".$path;
      $j--;
    }
    else {
      $path = "D".$path;
      $i--;
    }
  }
  return $path;
}
