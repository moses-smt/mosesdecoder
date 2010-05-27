<?php 

function diff_analysis() {
  global $task,$user,$setup,$id,$id2,$set;
  global $comment,$dir;

  head("Comparative Analysis");

  $c = $comment["$setup-$id"]->name;
  $c2 = $comment["$setup-$id2"]->name;
  print "<h4>$task ($user), Set $set, ";
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
            + '&setup=<?php print $setup ?>&id=<?php print $id ?>&id2=<?php print $id2 ?>&set=<?php print $set ?>'
            + '&sort=' + sort
            + '&count=' + count;
  new Ajax.Updater(field, url, { method: 'get' });
}
function ngram_diff(type,order,count,sort,smooth) {
  var url = '?analysis_diff=ngram_' + type + '_diff'
            + '&setup=<?php print $setup ?>&id=<?php print $id ?>&id2=<?php print $id2 ?>&set=<?php print $set ?>'
            + '&order=' + order
            + '&smooth=' + smooth
            + '&sort=' + sort
            + '&count=' + count;
  var field = (type == "precision" ? "nGramPrecision" : "nGramRecall") + order;
  new Ajax.Updater(field, url, { method: 'get' });
}
</script>
</head>
<body>
<div id="nGramSummary"><?php ngram_summary_diff() ?></div>
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
<div id="bleu">(loading...)</div>
<script>
diff('bleu','',5);
ngram_diff('precision',1,5,'',0);
ngram_diff('precision',2,5,'',0);
ngram_diff('precision',3,5,'',0);
ngram_diff('precision',4,5,'',0);
ngram_diff('recall',1,5,'',0);
ngram_diff('recall',2,5,'',0);
ngram_diff('recall',3,5,'',0);
ngram_diff('recall',4,5,'',0);
</script>
</body></html>
<?php
}

function ngram_summary_diff() {
  global $experiment,$evalset,$dir,$set,$id,$id2;

  // load data
  for($idx=0;$idx<2;$idx++) {
    $data = file("$dir/evaluation/$set.analysis.".($idx?$id2:$id)."/summary");
    for($i=0;$i<count($data);$i++) {
      $item = split(": ",$data[$i]);
      $info[$idx][$item[0]] = $item[1];
    }
  }

  print "<table cellpadding=5><tr><td>";
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

  print "</td><td valign=top>";

  printf("length-diff<br>%d (%+d)",$info[1]["precision-1-total"]-$info[1]["recall-1-total"],$info[1]["precision-1-total"]-$info[0]["precision-1-total"]);

  print "</td><td valign=top>";

  for($idx=0;$idx<2;$idx++) {
    $each_score = explode(" ; ",$experiment[$idx?$id2:$id]->result[$set]);
    for($i=0;$i<count($each_score);$i++) {
      if (preg_match('/([\d\(\)\.\s]+) (BLEU[\-c]*)/',$each_score[$i],$match) ||
          preg_match('/([\d\(\)\.\s]+) (IBM[\-c]*)/',$each_score[$i],$match)) {
	  $score[$match[2]][$idx] = $match[1];
      }
    }
  }
  foreach ($score as $name => $value) {
    $header .= "<td>$name</td>";
    $score_line .= "<td>".$score[$name][1]."</td>";
    $diff_line .= sprintf("<td>%+.2f</td>",$score[$name][1]-$score[$name][0]);
  }
  print "<table border=1><tr>".$header."</tr><tr>".$score_line."</tr><tr>".$diff_line."</tr></table>";

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
    $data = file("$dir/evaluation/$set.analysis.".($idx?$id2:$id)."/bleu-annotation");
    for($i=0;$i<count($data);$i++) {
      $item = split("\t",$data[$i]);
      $annotation[$item[1]]["bleu$idx"] = $item[0]; 
      $annotation[$item[1]]["system$idx"] = $item[2]; 
      $annotation[$item[1]]["reference"] = $item[3]; 
      $annotation[$item[1]]["id"] = $item[1];
    }
  }
  $data = array();

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

     print "<font size=-2>[".$line["id"].":".$line["bleu1"]."]</font> ";
     $matched1 = preg_replace('/D/',"",$matched);
     $matched_with_score1 = preg_replace('/D/',"",$matched_with_score);
     bleu_line_diff( $word_with_score1, $matched1, $matched_with_score1 );

     print "<font size=-2>[".$line["id"].":".$line["bleu0"]."]</font> ";     
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
    $data = file("$dir/evaluation/$set.analysis.".($idx?$id2:$id)."/n-gram-$type.$order");
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
