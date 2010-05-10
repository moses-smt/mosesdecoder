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
  new Ajax.Updater(field, url, { method: 'get', evalScripts: true });
}
function highlight_phrase(sentence,phrase) {
  var input = "input-"+sentence+"-"+phrase;
  $(input).setStyle({ borderWidth: '3px', borderColor: 'red' });
  var output = "output-"+sentence+"-"+phrase;
  $(output).setStyle({ borderWidth: '3px', borderColor: 'red' });
}
function show_word_info(sentence,cc,tc,te) {
  var info = "info-"+sentence;
  document.getElementById(info).innerHTML = ''+cc+' occurrences in corpus, '+tc+' distinct translations, translation entropy: '+te;
  $(info).setStyle({ opacity: 1 });
}
function lowlight_phrase(sentence,phrase) {
  var input = "input-"+sentence+"-"+phrase;
  $(input).setStyle({ borderWidth: '1px', borderColor: 'black' });
  var output = "output-"+sentence+"-"+phrase;
  $(output).setStyle({ borderWidth: '1px', borderColor: 'black' });
}
function hide_word_info(sentence) {
  var info = "info-"+sentence;
  $(info).setStyle({ opacity: 0 });
}
</script>
</head>
<body>
<div id="nGramSummary"><?php ngram_summary()  ?></div>
<div id="CoverageDetails"></div>
<div id="PrecisionRecallDetails"></div>
<div id="bleu">(loading...)</div>
<script language="javascript">
show('bleu','',5);
</script>
</body></html>
<?php
}

function precision_recall_details() {
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
ngram_show('precision',1,5,'',0);
ngram_show('precision',2,5,'',0);
ngram_show('precision',3,5,'',0);
ngram_show('precision',4,5,'',0);
ngram_show('recall',1,5,'',0);
ngram_show('recall',2,5,'',0);
ngram_show('recall',3,5,'',0);
ngram_show('recall',4,5,'',0);
</script>
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

  print "<A HREF=\"javascript:generic_show('PrecisionRecallDetails','')\">details</A> ";

 print "</td><td valign=top valign=top align=center bgcolor=#eeeeee>";

  $each_score = explode(" ; ",$experiment[$id]->result[$set]);
  $header = "";
  $score_line = "";
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
    print "</td><td valign=top align=center bgcolor=#eeeeee>";
    print "<div id=\"CoverageSummary\">";
    coverage_summary();
    print "</div>";
  }

  // phrase segmentation
  if (file_exists("$dir/evaluation/$set.analysis.$id/segmentation")) {
    print "</td><td valign=top align=center bgcolor=#eeeeee>";
    print "<div id=\"SegmentationSummary\">";
    segmentation_summary();
    print "</div>";
  }
  print "</td></tr></table>";
}

function coverage_details() {
  global $dir,$set,$id;

  $count = array(); $token = array();
  foreach (array("ttable","corpus") as $corpus) {
    foreach (array("token","type") as $b) {
      for($i=0;$i<=7;$i++) {
        foreach (array("6+","2-5","1","0") as $range) {
          $count[$corpus][$b][$i][$range] = 0;
        }
        $total[$corpus][$b][$i] = 0;
      }
    }
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

  if (array_key_exists("by",$_GET)) { $by = $_GET['by']; }
  else { $by = 'token'; }

  $total = array(); $count = array();
  foreach (array("ttable","corpus") as $corpus) {
    foreach (array("token","type") as $b) {
      foreach (array("6+","2-5","1","0") as $c) {
        $count[$corpus][$b][$c] = 0;
      }
      $total[$corpus][$b] = 0;
    }
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
  print "<div id=\"CoverageDetailsLink\"<A HREF=\"javascript:generic_show('CoverageDetails','')\">details</A> ";
}

function segmentation_summary() {
  global $dir,$set,$id;

  if (array_key_exists("by",$_GET)) { $by = $_GET['by']; }
  else { $by = 'word'; }

  $count = array();
  for($i=0;$i<=4;$i++) {
    $count[$i] = array();
    for($j=0;$j<=4;$j++) {
      $count[$i][$j] = 0;
    }
  }

  $data = file("$dir/evaluation/$set.analysis.$id/segmentation");
  $total = 0; 
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
      if (array_key_exists($in,$count) &&
          array_key_exists($out,$count[$in])) {
	  $c = $count[$in][$out];
      } 
      else { $c = 0; }
      printf("<td align=right nowrap>%d (%.1f%s)</td>",$c,100*$c/$total,"%");
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

  #print "display <A HREF=\"\">fullscreen</A> ";

  $count = $_GET['count'];
  if ($count == 0) { $count = 5; }
  print "showing $count ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',5+$count)\">more</A> ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',9999)\">all</A> ";

  print "</font><BR>\n";

  sentence_annotation();
  print "<p align=center><A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',5+$count)\">5 more</A> | ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',10+$count)\">10 more</A> | ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',20+$count)\">20 more</A> | ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',50+$count)\">50 more</A> | ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',100+$count)\">100 more</A> | ";
  print "<A HREF=\"javascript:show('bleu','" . $_GET['sort'] . "',9999)\">all</A> ";
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
       print "<div id=\"info-$i\" style=\"border-color:black; background:#ffff80; opacity:0; width:100%; border:1px;\">8364 occ. in corpus, 56 translations, entropy: 5.54</div>\n";
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

function input_annotation($sentence,$input,$segmentation) {
  list($words,$coverage_vector) = split("\t",$input);

  # get information from line in input annotation file
  foreach (split(" ",$coverage_vector) as $item) {
    if (preg_match("/[\-:]/",$item)) {
      list($from,$to,$corpus_count,$ttable_count,$ttable_entropy) = preg_split("/[\-:]/",$item);
      $coverage[$from][$to]["corpus_count"] = $corpus_count;
      $coverage[$from][$to]["ttable_count"] = $ttable_count;
      $coverage[$from][$to]["ttable_entropy"] = $ttable_entropy;
    }
  }
  $word = split(" ",$words);

  # compute the display level for each input phrase
  for($j=0;$j<count($word);$j++) {
    $box[] = array();
    $separable[] = 1;
  }
  $max_level = 0;
  for($length=1;$length<=7;$length++) {
    for($from=0;$from<count($word)-($length-1);$from++) {
      $to = $from + ($length-1);
      if (array_key_exists($from,$coverage) &&
          array_key_exists($to,$coverage[$from]) &&
          array_key_exists("corpus_count",$coverage[$from][$to])) {
        $level=0;
	$available = 0;
        while(!$available) {
	  $available = 1;
	  $level++;
	  for($j=$from;$j<=$to;$j++) {
            if (array_key_exists($level,$box) &&
                array_key_exists($j,$box[$level])) {
	      $available = 0;
	    }
          }
        }
	for($j=$from;$j<=$to;$j++) {
	  $box[$level][$j] = $to;
	}
	$max_level = max($max_level,$level);
	for($j=$from+1;$j<=$to;$j++) {
	  $separable[$j] = 0;
	}
      }
    }
  }
  $separable[count($word)] = 1;

  # display input phrases
  $sep_start = 0;
  for($sep_end=1;$sep_end<=count($word);$sep_end++) {
    if ($separable[$sep_end] == 1) {
      # one table for each separable block
      print "<table cellpadding=1 cellspacing=0 border=0 style=\"display: inline;\">";
      for($level=$max_level;$level>=1;$level--) {
        # rows for phrase display
	print "<tr style=\"height:5px;\">";
	for($from=$sep_start;$from<$sep_end;$from++) {
	  if (array_key_exists($from,$box[$level])) {
	    $to = $box[$level][$from];
            $size = $to - $from + 1;
	    if ($size == 1) {
	      print "<td><div style=\"height:0px; opacity:0; position:relative; z-index:-9;\">".$word[$from];
            }
	    else {
		$color = coverage_color($coverage[$from][$to]);
		$phrase = "";
		$highlightwords = "";
                $lowlightwords = "";
		for($j=$from;$j<=$to;$j++) {
		  if ($j>$from) { $phrase .= " "; }
		  $phrase .= $word[$j];
                  $highlightwords .= " document.getElementById('inputword-$sentence-$j').style.backgroundColor='#ffff80';";
                  $lowlightwords .= " document.getElementById('inputword-$sentence-$j').style.backgroundColor='".coverage_color($coverage[$j][$j])."';";
		}
	        print "<td colspan=$size><div style=\"background-color: $color; height:3px;\" onmouseover=\"show_word_info($sentence,".$coverage[$from][$to]["corpus_count"].",".$coverage[$from][$to]["ttable_count"].",".$coverage[$from][$to]["ttable_entropy"]."); this.style.backgroundColor='#ffff80';$highlightwords\" onmouseout=\"hide_word_info($sentence); this.style.backgroundColor='$color';$lowlightwords\">";
            }
            print "</div></td>";
	    $from += $size-1;
	  }
	  else {
	    print "<td><div style=\"height:".($from==$to ? 0 : 3)."px;\"></div></td>";
	  }
	}
	print "</tr>\n";
      }
      # display input words
      print "<tr><td colspan=".($sep_end-$sep_start)."><div style=\"position:relative; z-index:1;\">";
      for($j=$sep_start;$j<$sep_end;$j++) {
        if ($segmentation && array_key_exists($j,$segmentation["input_start"])) {
          $id = $segmentation["input_start"][$j];  
          print "<span id=\"input-$sentence-$id\" style=\"border-color:#000000; border-style:solid; border-width:1px;\" onmouseover=\"highlight_phrase($sentence,$id);\" onmouseout=\"lowlight_phrase($sentence,$id);\">";
        }
        if (array_key_exists($j,$coverage)) {
          $color = coverage_color($coverage[$j][$j]);
          $cc = $coverage[$j][$j]["corpus_count"];
          $tc = $coverage[$j][$j]["ttable_count"];
          $te = $coverage[$j][$j]["ttable_entropy"];
        }
        else { # unknown words
	  $color = '#ffffff';
          $cc = 0; $tc = 0; $te = 0;
        }
        print "<span id=\"inputword-$sentence-$j\" style=\"background-color: $color;\" onmouseover=\"show_word_info($sentence,$cc,$tc,$te); this.style.backgroundColor='#ffff80';\" onmouseout=\"hide_word_info($sentence);  this.style.backgroundColor='$color';\">$word[$j]</span>";
        if ($segmentation && array_key_exists($j,$segmentation["input_end"])) {
          print "</span>";
        }
        print " ";
      }
      print "</div></td></tr>\n";
      print "</table>\n";
      $sep_start = $sep_end;
    }
  }
  print "<br>";
}

function coverage_color($phrase) {
  $corpus_count = 255 - 10 * log(1 + $phrase["corpus_count"]);
  if ($corpus_count < 128) { $corpus_count = 128; }
  $cc_color = dechex($corpus_count / 16) . dechex($corpus_count % 16);

  $ttable_count = 255 - 20 * log(1 + $phrase["ttable_count"]);
  if ($ttable_count < 128) { $ttable_count = 128; }
  $tc_color = dechex($ttable_count / 16) . dechex($ttable_count % 16);

  $ttable_entropy = 255 - 32 * $phrase["ttable_entropy"];
  if ($ttable_entropy < 128) { $ttable_entropy = 128; }
  $te_color = dechex($ttable_entropy / 16) . dechex($ttable_entropy % 16);

//  $color = "#". $cc_color . $te_color .  $tc_color; # reddish browns with some green
//  $color = "#". $cc_color . $tc_color .  $te_color; # reddish brown with some blueish purple
  $color = "#". $te_color . $cc_color .  $tc_color; # pale green towards red
//  $color = "#". $te_color . $tc_color .  $cc_color; # pale purple towards red
//  $color = "#". $tc_color . $te_color .  $cc_color; // # blue-grey towards green
//  $color = "#". $tc_color . $cc_color .  $te_color; // # green-grey towards blue

  return $color;
}

function bleu_annotation($i,$system,$segmentation) {
  #$color = array("#FFC0C0","#FFC0FF","#C0C0FF","#C0FFFF","#C0FFC0");
  $color = array("#c0c0c0","#c0c0ff","#a0a0ff","#8080ff","#4040ff");
  $word = split(" ",$system);

  for($j=0;$j<count($word);$j++) {
    list($surface,$correct) = split("\|", $word[$j]);
    if ($segmentation && array_key_exists($j,$segmentation["output_start"])) {
      $id = $segmentation["output_start"][$j];
      print "<span id=\"output-$i-$id\" style=\"border-color:#000000; border-style:solid; border-width:1px;\" onmouseover=\"highlight_phrase($i,$id);\" onmouseout=\"lowlight_phrase($i,$id);\">";
    }
    print "<span style=\"background-color: $color[$correct]\">$surface</span>";
    if ($segmentation && array_key_exists($j,$segmentation["output_end"])) {
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
