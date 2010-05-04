<?php 

function show_analysis() {
  global $task,$user,$setup,$id,$set;
  global $dir;

  head("Analysis: $task ($user), Set $set, Run $id");

?><script language="javascript" src="/javascripts/prototype.js"></script>
<script language="javascript" src="/javascripts/scriptaculous.js"></script>
<script>
function show(field,sort,count) {
  var url = '?analysis=' + field + '_show'
            + '&setup=<?php print $setup ?>&id=<?php print $id ?>&set=<?php print $set ?>'
            + '&sort=' + sort
            + '&count=' + count;
  new Ajax.Updater(field, url, { method: 'get' });
}
function ngram_show(type,order,count,sort,smooth) {
  var url = '?analysis=ngram_' + type + '_show'
            + '&setup=<?php print $setup ?>&id=<?php print $id ?>&set=<?php print $set ?>'
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
<div id="nGramSummary"><?php ngram_summary()  ?></div>
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
show('bleu','',5);
ngram_show('precision',1,5,'',0);
ngram_show('precision',2,5,'',0);
ngram_show('precision',3,5,'',0);
ngram_show('precision',4,5,'',0);
ngram_show('recall',1,5,'',0);
ngram_show('recall',2,5,'',0);
ngram_show('recall',3,5,'',0);
ngram_show('recall',4,5,'',0);
</script>
</body></html>
<?php
}

function ngram_summary() {
  global $experiment,$evalset,$dir,$set,$id;

  // load data
  $data = file("$dir/evaluation/$set.analysis.$id/summary");
  for($i=0;$i<count($data);$i++) {
    $item = split(": ",$data[$i]);
    $info[$item[0]] = $item[1];
  }

  print "<table cellpadding=5><tr><td>";
  //foreach (array("precision","recall") as $type) {
  $type = "precision";
  print "<table><tr><td>$type</td><td>1-gram</td><td>2-gram</td><td>3-gram</td><td>4-gram</td></tr>\n";
  printf("<tr><td>correct</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>\n",
	 $info["$type-1-correct"],
	 $info["$type-2-correct"],
	 $info["$type-3-correct"],
	 $info["$type-4-correct"]);    
  printf("<tr><td>&nbsp;</td><td>%.1f%s</td><td>%.1f%s</td><td>%.1f%s</td><td>%.1f%s</td></tr>\n",
	 $info["$type-1-correct"]/$info["$type-1-total"]*100,'%',
	 $info["$type-2-correct"]/$info["$type-2-total"]*100,'%',
	 $info["$type-3-correct"]/$info["$type-3-total"]*100,'%',
	 $info["$type-4-correct"]/$info["$type-4-total"]*100,'%');
  printf("<tr><td>wrong</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>\n",
	 $info["$type-1-total"]-$info["$type-1-correct"],
	 $info["$type-2-total"]-$info["$type-2-correct"],
	 $info["$type-3-total"]-$info["$type-3-correct"],
	 $info["$type-4-total"]-$info["$type-4-correct"]);
  print "</table>";
  //}

  print "</td><td valign=top>";

  printf("length-diff<br>%d",$info["precision-1-total"]-$info["recall-1-total"]);

  print "</td><td valign=top>";

  $each_score = explode(" ; ",$experiment[$idx?$id2:$id]->result[$set]);
  for($i=0;$i<count($each_score);$i++) {
    if (preg_match('/([\d\(\)\.\s]+) (BLEU[\-c]*)/',$each_score[$i],$match) ||
        preg_match('/([\d\(\)\.\s]+) (IBM[\-c]*)/',$each_score[$i],$match)) {
      $header .= "<td>$match[2]</td>";
      $score_line .= "<td>$match[1]</td>";
    }
  }
  print "<table border=1><tr>".$header."</tr><tr>".$score_line."</tr></table>";

  print "</td><tr><table>";

}

function bleu_show() {
  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }

  print "<b>annotated sentences</b><br><font size=-1>sorted by ";

  if ($_GET['sort'] == "order" || $_GET['sort'] == "") {
    print "order ";
  }
  else {
    print "<A HREF=\"javascript:show('bleu','order',$count)\">order</A> ";
  }

  if ($_GET['sort'] == "best") {
    print "order ";
  }
  else {
    print "<A HREF=\"javascript:show('bleu','best',$count)\">best</A> ";
  }

  if ($_GET['sort'] == "worst") {
    print "order ";
  }
  else {
    print "<A HREF=\"javascript:show('bleu','worst',$count)\">worst</A> ";
  }

  print "display <A HREF=\"\">fullscreen</A> ";

  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }
  print "showing $count ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',5+$count)\">more</A> ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',9999)\">all</A> ";

  print "</font><BR>\n";

  bleu_annotation();
}

function bleu_annotation() {
  global $set,$id,$dir;
  $color = array("#FFC0C0","#FFC0FF","#C0C0FF","#C0FFFF","#C0FFC0");

  // load data
  $data = file("$dir/evaluation/$set.analysis.$id/bleu-annotation");
  for($i=0;$i<count($data);$i++) {
     $item = split("\t",$data[$i]);
     $line["bleu"] = $item[0]; 
     $line["id"] = $item[1]; 
     $line["system"] = $item[2]; 
     $line["reference"] = $item[3]; 
     $annotation[] = $line;
  }

  // sort
  global $sort;
  $sort = $_GET['sort'];
  if ($sort == '') {
    $sort = "order";
  }
  function cmp($a, $b) {
    global $sort;
    if ($sort == "order") {
      $a_idx = $a["id"];
      $b_idx = $b["id"];
    }
    else if ($sort == "worst") {
      $a_idx = $a["bleu"];
      $b_idx = $b["bleu"];
    }
    else if ($sort == "best") {
      $a_idx = -$a["bleu"];
      $b_idx = -$b["bleu"];
    }

    if ($a_idx == $b_idx) {
        return 0;
    }
    return ($a_idx < $b_idx) ? -1 : 1;
  }

  usort($annotation, 'cmp');

  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }

  // display
  for($i=0;$i<$count && $i<count($annotation);$i++) {
     $line = $annotation[$i]; 
     print "<font size=-2>[".$line["id"].":".$line["bleu"]."]</font> ";
     $word = split(" ",$line["system"]);
     for($j=0;$j<count($word);$j++) {
       list($surface,$correct) = split("\|", $word[$j]);
       print "<span style=\"background-color: $color[$correct]\">$surface</span> ";
     }
     print "<br>".$line["reference"]."<hr>";
  }
}

function ngram_show($type) {
  global $set,$id,$dir;

  // load data
  $order = $_GET['order'];
  $data = file("$dir/evaluation/$set.analysis.$id/n-gram-$type.$order");
  for($i=0;$i<count($data);$i++) {
     $item = split("\t",$data[$i]);
     $line["total"] = $item[0]; 
     $line["correct"] = $item[1];
     $line["ngram"] = $item[2];
     $ngram[] = $line;
  }

  // sort option
  $sort = $_GET['sort'];
  $smooth = $_GET['smooth'];
  if ($sort == '') {
    $sort = 'ratio_worst';
    $smooth = 1;
  }
  
  // sort index
  for($i=0;$i<count($ngram);$i++) {
    if ($sort == "abs_worst") {
      $ngram[$i]["index"] = $ngram[$i]["correct"] - $ngram[$i]["total"];
    }
    else if ($sort == "ratio_worst") {
      $ngram[$i]["index"] = ($ngram[$i]["correct"] + $smooth) / ($ngram[$i]["total"] + $smooth);
    }
  }

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

  print "<B>$order-gram $type</B><br><font size=-1>sorted by ";
  if ($sort == "ratio_worst") {
    print "ratio ";
    print "smooth-$smooth ";
    print "<A HREF=\"javascript:ngram_show('$type',$order,$count,'ratio_worst',$smooth+1)\">+</A> ";
    print "<A HREF=\"javascript:ngram_show('$type',$order,$count,'ratio_worst',$smooth-1)\">-</A> ";
  }
  else {
    print "<A HREF=\"javascript:ngram_show('$type',$order,$count,'ratio_worst',1)\">ratio</A> ";
  }
  if ($sort == "abs_worst") {
    print "absolute ";
  }
  else {
    print "<A HREF=\"javascript:ngram_show('$type',$order,$count,'abs_worst',0)\">absolute</A> ";
  }
  print "showing $count ";
  if ($count < 9999) {
    print "<A HREF=\"javascript:ngram_show('$type',$order,$count+5,'$sort',$smooth)\">more</A> ";
    print "<A HREF=\"javascript:ngram_show('$type',$order,9999,'$sort',$smooth)\">all</A> ";
  }
  else {
    print "<A HREF=\"javascript:ngram_show('$type',$order,5,'$sort',$smooth)\">top5</A> ";
  }
  print "</font><br>\n";

  print "<table width=100%>\n";
  print "<tr><td>$order-gram</td><td>ok</td><td>x</td><td>ratio</td></tr>\n";
  for($i=0;$i<$count && $i<count($ngram);$i++) {
    $line = $ngram[$i];
    print "<tr><td>".$line["ngram"]."</td>";
    print "<td>".$line["correct"]."</td>";
    print "<td>".($line["total"]-$line["correct"])."</td>";
    printf("<td>%.3f</td></tr>",$line["correct"]/$line["total"]);
  }
  print "</table>\n";
}
