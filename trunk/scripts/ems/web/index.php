<?php

require("lib.php");
require("overview.php");
require("analysis.php");
require("analysis_diff.php");
require("diff.php");

function head($title) {
  print "<HTML><HEAD><TITLE>$title</TITLE></HEAD>\n<BODY><H2>$title</H2>\n";
}

if ($_POST["setup"] || $_GET["setup"]) {
  load_experiment_info();
  load_comment();

  if ($_GET["show"]) { show(); }
  else if ($_GET["diff"]) { diff(); }
  else if ($_GET["analysis"]) { 
    $action = $_GET["analysis"];
    $set = $_GET["set"];
    $id = $_GET["id"];
    if ($action == "show") { show_analysis(); }
    else if ($action == "bleu_show") { bleu_show(); }
    else if ($action == "ngram_precision_show") { ngram_show("precision");}
    else if ($action == "ngram_recall_show") { ngram_show("recall"); }
    else if ($action == "CoverageSummary_show") { coverage_summary(); }
    else if ($action == "CoverageDetails_show") { coverage_details(); }
    else if ($action == "SegmentationSummary_show") { segmentation_summary(); }
    else { print "ERROR! $action"; }
  }
  else if ($_GET["analysis_diff_home"]) {
    $set = $_GET["analysis_diff_home"];
    while (list($parameter,$value) = each($_GET)) {
      if (preg_match("/analysis\-(\d+)\-(.+)/",$parameter,$match)) {
        if ($match[2] == $set) {
          $id_array[] = $match[1];
        }
      }      
    }
    if (count($id_array) != 2) {
      print "ERROR: comp 2!";
      exit();
    }
    $id = $id_array[0];
    $id2 = $id_array[1];
    if ($id>$id2) { $i=$id; $id=$id2; $id2=$i; }
    diff_analysis();
  }
  else if ($_GET["analysis_diff"]) {
    $action = $_GET["analysis_diff"];
    $set = $_GET["set"];
    $id = $_GET["id"];
    $id2 = $_GET["id2"];
    if ($action == "bleu_diff") { bleu_diff(); }
    else if ($action == "ngram_precision_diff") { ngram_diff("precision");}
    else if ($action == "ngram_recall_diff") { ngram_diff("recall"); }
    else { print "ERROR! $action"; }
  }
  else { overview(); }
}
else {
  setup();
}

print "</BODY></HTML>\n";
