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
function generic_show(field,parameters) {
  var url = '?analysis=' + field + '_show'
            + '&setup=<?php print $setup ?>&id=<?php print $id ?>&set=<?php print $set ?>'
            + '&' + parameters;
  new Ajax.Updater(field, url, { method: 'get' });
}
function highlight_phrase(sentence,phrase) {
  var input = "input-"+sentence+"-"+phrase;
  $(input).setStyle({ borderWidth: '3px', borderColor: 'red' });
  var output = "output-"+sentence+"-"+phrase;
  $(output).setStyle({ borderWidth: '3px', borderColor: 'red' });
}
function lowlight_phrase(sentence,phrase) {
  var input = "input-"+sentence+"-"+phrase;
  $(input).setStyle({ borderWidth: '1px', borderColor: 'black' });
  var output = "output-"+sentence+"-"+phrase;
  $(output).setStyle({ borderWidth: '1px', borderColor: 'black' });
}
</script>
</head>
<body>
<div id="nGramSummary"><?php ngram_summary()  ?></div>
<div id="CoverageDetails"></div>
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

  print "<table cellspacing=5 width=100%><tr><td valign=top align=center bgcolor=#eeeeee>";
  //foreach (array("precision","recall") as $type) {
  print "<b>Precision</b>\n";
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

  print "</td><td valign=top valign=top align=center bgcolor=#eeeeee>";

  $each_score = explode(" ; ",$experiment[$idx?$id2:$id]->result[$set]);
  for($i=0;$i<count($each_score);$i++) {
    if (preg_match('/([\d\(\)\.\s]+) (BLEU[\-c]*)/',$each_score[$i],$match) ||
        preg_match('/([\d\(\)\.\s]+) (IBM[\-c]*)/',$each_score[$i],$match)) {
      $header .= "<td>$match[2]</td>";
      $score_line .= "<td>$match[1]</td>";
    }
  }
  print "<b>Metrics</b><table border=1><tr>".$header."</tr><tr>".$score_line."</tr></table>";

  printf("<p>length-diff: %d (%.1f%s)",$info["precision-1-total"]-$info["recall-1-total"],($info["precision-1-total"]-$info["recall-1-total"])/$info["recall-1-total"]*100,"%");

  // coverage
  if (file_exists("$dir/evaluation/$set.analysis.$id/corpus-coverage-summary")) {
    print "</td><td valign=top valign=top align=center bgcolor=#eeeeee>";
    print "<div id=\"CoverageSummary\">";
    coverage_summary();
    print "</div>";
  }

  // phrase segmentation
  if (file_exists("$dir/evaluation/$set.analysis.$id/segmentation")) {
    print "</td><td valign=top valign=top align=center bgcolor=#eeeeee>";
    print "<div id=\"SegmentationSummary\">";
    segmentation_summary();
    print "</div>";
  }
  print "</td></tr></table>";
}

function coverage_details() {
  global $dir,$set,$id;

  foreach (array("ttable","corpus") as $corpus) {
    $data = file("$dir/evaluation/$set.analysis.$id/$corpus-coverage-summary");
    for($i=0;$i<count($data);$i++) {
      $item = split("\t",$data[$i]);
      if ($item[1]>5) {
        $count[$corpus]["type"][$item[0]]["6+"] += $item[2];
	$count[$corpus]["token"][$item[0]]["6+"] += $item[3];
      }
      else if ($item[1]>1) {
        $count[$corpus]["type"][$item[0]]["2-5"] += $item[2];
	$count[$corpus]["token"][$item[0]]["2-5"] += $item[3];
      }
      else if ($item[1]==1) {
        $count[$corpus]["type"][$item[0]]["1"] += $item[2];
	$count[$corpus]["token"][$item[0]]["1"] += $item[3];
      }
      else {
        $count[$corpus]["type"][$item[0]]["0"] += $item[2];
	$count[$corpus]["token"][$item[0]]["0"] += $item[3];
      }
      $total[$corpus]["type"][$item[0]] += $item[2];
      $total[$corpus]["token"][$item[0]] += $item[3];
    }
  }
  print "<b>coverage</b><br>\n";
  print "<table width=100%><tr>";
  foreach (array("token","type") as $by) {
    for($i=1;$i<=4;$i++) {
      print "<td align=center><b>$i-gram ($by)</b><br>\n";
      print "<table><tr><td></td><td>model</td><td>corpus</td></tr>\n";
      foreach (array("0","1","2-5","6+") as $range) {
        print "<tr><td>$range</td>";
        foreach (array("ttable","corpus") as $corpus) {
          printf("<td align=right nowrap>%d (%.1f%s)</td>",$count[$corpus][$by][$i][$range],100*$count[$corpus][$by][$i][$range]/($total[$corpus][$by][$i]+0.0001),"%");
        }
        print "</tr>\n";
      }
      print "</table></td>\n";
    }
    print "</tr><tr>";
  }
  print "</tr></table>\n";

  $data = file("$dir/evaluation/$set.analysis.$id/ttable-unknown");
  for($i=0;$i<count($data);$i++) {
    list($word,$count) = split("\t",$data[$i]);
    $item["word"] = $word;
    $item["count"] = rtrim($count);
    $unknown[] = $item;
  }

  function cmp($a,$b) {
    if ($a["count"] > $b["count"]) {
      return -1;
    }
    else if ($a["count"] < $b["count"]) {
      return 1;
    }
    else {
      return strcmp($a["word"],$b["word"]);
    }
  }

  usort($unknown, 'cmp');

  print "<b>unknown words</b><br>\n";
  print "<table><tr><td valign=top><table>";
  $state = 5;
  foreach ($unknown as $item) {
    if ($item["count"] < $state) {
      if ($state == 5) { print "</table>"; }
      print "</td><td valign=top><b>".$item["count"].":</b> ";
      $state = $item["count"];
      if ($state == 1) { print "<font size=-1>"; }
    }
    else if ($state<5) {
      print ", ";
    }
    if ($state == 5) {
      print "<tr><td>".$item["count"]."</td><td>".$item["word"]."</td></tr>";
    }
    else {
      print $item["word"];
    }
  }
  print "</font></td></tr></table>\n";
}

function coverage_summary() {
  global $dir,$set,$id,$corpus;
  $by = $_GET['by'];
  if ($by == '') { $by = 'token'; }

  $total = array(); $count = array();
  foreach (array("ttable","corpus") as $corpus) {
    $data = file("$dir/evaluation/$set.analysis.$id/$corpus-coverage-summary");
    for($i=0;$i<count($data);$i++) {
      $item = split("\t",$data[$i]);
      if ($item[0] == 1) {
        if ($item[1]>5) {
          $count[$corpus]["type"]["6+"] += $item[2];
	  $count[$corpus]["token"]["6+"] += $item[3];
        }
        else if ($item[1]>1) {
          $count[$corpus]["type"]["2-5"] += $item[2];
  	  $count[$corpus]["token"]["2-5"] += $item[3];
        }
        else if ($item[1]==1) {
          $count[$corpus]["type"]["1"] += $item[2];
	  $count[$corpus]["token"]["1"] += $item[3];
        }
        else {
          $count[$corpus]["type"]["0"] += $item[2];
	  $count[$corpus]["token"]["0"] += $item[3];
        }
        $total[$corpus]["type"] += $item[2];
        $total[$corpus]["token"] += $item[3];
      }
    }
  }

  print "<b>Coverage</b>\n";
  print "<table><tr><td></td><td>model</td><td>corpus</td></tr>\n";
  foreach (array("0","1","2-5","6+") as $range) {
    print "<tr><td>$range</td>";
    foreach (array("ttable","corpus") as $corpus) {
      printf("<td align=right nowrap>%d (%.1f%s)</td>",$count[$corpus][$by][$range],100*$count[$corpus][$by][$range]/($total[$corpus][$by]+0.0001),"%");
    }
    print "</tr>\n";
  }
  print "</table>\n";
  if ($by == 'token') { print "by token"; } else {
    print "<A HREF=\"javascript:generic_show('CoverageSummary','by=token')\">by token</A> ";
  }
  print " / ";
  if ($by == 'type') { print "by type"; } else {
    print "<A HREF=\"javascript:generic_show('CoverageSummary','by=type')\">by type</A> ";
  }
  print " / ";
  print "<A HREF=\"javascript:generic_show('CoverageDetails','')\">details</A> ";
}


function segmentation_summary() {
  global $dir,$set,$id;
  $by = $_GET['by'];
  if ($by == '') { $by = 'word'; }

  $data = file("$dir/evaluation/$set.analysis.$id/segmentation");
  $total = 0; $count = array();
  for($i=0;$i<count($data);$i++) {
    list($in,$out,$c) = split("\t",$data[$i]);
    if ($by == "word") { $c *= $in; }
    if ($in>4)  { $in  = 4; }
    if ($out>4) { $out = 4; }
    $total += $c;
    $count[$in][$out] += $c;
  }

  print "<b>Phrase Segmentation</b><br>\n";
  print "<table>";
  print "<tr><td></td><td align=center>1</td><td align=center>2</td><td align=center>3</td><td align=center>4+</td></tr>";
  for($in=1;$in<=4;$in++) {
    print "<tr><td nowrap>$in".($in==4?"+":"")." to</td>";
    for($out=1;$out<=4;$out++) {
      printf("<td align=right nowrap>%d (%.1f%s)</td>",$count[$in][$out],100*$count[$in][$out]/$total,"%");
    }
    print "</tr>";
  }
  print "</table>\n";
  if ($by == 'word') { print "by word"; } else {
    print "<A HREF=\"javascript:generic_show('SegmentationSummary','by=word')\">by word</A> ";
  }
  print " / ";
  if ($by == 'phrase') { print "by phrase"; } else {
    print "<A HREF=\"javascript:generic_show('SegmentationSummary','by=phrase')\">by phrase</A> ";
  }
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

  sentence_annotation();
}

function sentence_annotation() {
  global $set,$id,$dir;

  // load data
  $data = file("$dir/evaluation/$set.analysis.$id/bleu-annotation");
  for($i=0;$i<count($data);$i++) {
     $item = split("\t",$data[$i]);
     $line["bleu"] = $item[0]; 
     $line["id"] = $item[1]; 
     $line["system"] = $item[2]; 
     $line["reference"] = $item[3]; 
     $bleu[] = $line;
  }

  if (file_exists("$dir/evaluation/$set.analysis.$id/input-annotation")) {
    $input = file("$dir/evaluation/$set.analysis.$id/input-annotation");
  }

  if (file_exists("$dir/evaluation/$set.analysis.$id/segmentation-annotation")) {
    $data = file("$dir/evaluation/$set.analysis.$id/segmentation-annotation");
   for($i=0;$i<count($data);$i++) {
      $id = 0;
      foreach (split(" ",$data[$i]) as $item) {
	list($in_start,$in_end,$out_start,$out_end) = split(":",$item);
	$id++;
        $segmentation[$i]["input_start"][$in_start]=$id;
        $segmentation[$i]["input_end"][$in_end]=$id;
        $segmentation[$i]["output_start"][$out_start]=$id;
        $segmentation[$i]["output_end"][$out_end+0]=$id;
      }
    }
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

  usort($bleu, 'cmp');

  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }

  // display
  for($i=0;$i<$count && $i<count($bleu);$i++) {
     $line = $bleu[$i]; 
     if ($input) {
       print "<font size=-2>[#".$line["id"]."]</font> ";
       input_annotation($line["id"],$input[$line["id"]],$segmentation[$line["id"]]);
       print "<font size=-2>[".$line["bleu"]."]</font> ";
     }
     else {
       print "<font size=-2>[".$line["id"].":".$line["bleu"]."]</font> ";
     }
     bleu_annotation($line["id"],$line["system"],$segmentation[$line["id"]]);
     print "<br>".$line["reference"]."<hr>";
  }
}

function input_annotation($i,$input,$segmentation) {
  list($sentence,$coverage_vector) = split("\t",$input);

  foreach (split(" ",$coverage_vector) as $item) {
    list($from,$to,$corpus_count,$ttable_count,$ttable_entropy) = preg_split("/[\-:]/",$item);
    $coverage[$from][$to]["corpus_count"] = $corpus_count;
    $coverage[$from][$to]["ttable_count"] = $ttable_count;
    $coverage[$from][$to]["ttable_entropy"] = $ttable_entropy;
  }
  $word = split(" ",$sentence);
  for($j=0;$j<count($word);$j++) {

    $corpus_count = 255 - 20 * log(1 + $coverage[$j][$j]["corpus_count"]);
    if ($corpus_count < 128) { $corpus_count = 128; }
    $cc_color = dechex($corpus_count / 16) . dechex($corpus_count % 16);

    $ttable_count = 255 - 20 * log(1 + $coverage[$j][$j]["ttable_count"]);
    if ($ttable_count < 128) { $ttable_count = 128; }
    $tc_color = dechex($ttable_count / 16) . dechex($ttable_count % 16);

    $ttable_entropy = 255 - 16 * $coverage[$j][$j]["ttable_entropy"];
    if ($ttable_entropy < 128) { $ttable_entropy = 128; }
    $te_color = dechex($ttable_entropy / 16) . dechex($ttable_entropy % 16);
    
//    $color = "#". $cc_color . $te_color .  $tc_color; # pale green
//    $color = "#". $cc_color . $tc_color .  $te_color; # pale blue
//    $color = "#". $te_color . $cc_color .  $tc_color; # red towards purple
//    $color = "#". $te_color . $tc_color .  $cc_color; # red with some orange
//    $color = "#". $tc_color . $te_color .  $cc_color; // # green with some yellow
    $color = "#". $tc_color . $cc_color .  $te_color; // # blue-purple
    if ($segmentation && $segmentation["input_start"][$j]) {
      $id = $segmentation["input_start"][$j];
      print "<span id=\"input-$i-$id\" style=\"border-color:#000000; border-style:solid; border-width:1px;\" onmouseover=\"highlight_phrase($i,$id);\" onmouseout=\"lowlight_phrase($i,$id);\">";
    }
    print "<span style=\"background-color: $color;\">$word[$j]</span>";
    if ($segmentation && $segmentation["input_end"][$j]) {
      print "</span>";
    }
    print " ";
  }
  print "<br>";
}

function bleu_annotation($i,$system,$segmentation) {
  $color = array("#FFC0C0","#FFC0FF","#C0C0FF","#C0FFFF","#C0FFC0");
  $word = split(" ",$system);

  for($j=0;$j<count($word);$j++) {
    list($surface,$correct) = split("\|", $word[$j]);
    if ($segmentation && $segmentation["output_start"][$j]) {
      $id = $segmentation["output_start"][$j];
      print "<span id=\"output-$i-$id\" style=\"border-color:#000000; border-style:solid; border-width:1px;\" onmouseover=\"highlight_phrase($i,$id);\" onmouseout=\"lowlight_phrase($i,$id);\">";
    }
    print "<span style=\"background-color: $color[$correct]\">$surface</span>";
    if ($segmentation && $segmentation["output_end"][$j]) {
      print "</span>";
    }
    print " ";
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
