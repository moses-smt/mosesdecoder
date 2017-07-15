<?php

/*
This file is part of moses.  Its use is licensed under the GNU Lesser General
Public License version 2.1 or, at your option, any later version.
*/

function load_experiment_info() {
  global $dir,$task,$user,$setup;
  global $evalset;
  global $experiment;

  if (array_key_exists("setup",$_POST)) { $setup = $_POST["setup"]; }
  if (array_key_exists("setup",$_GET)) { $setup= $_GET["setup"]; }
  $all_setup = file("setup");
  while (list($id,$line) = each($all_setup)) {
    $info = explode(";",$line);
    if ($info[0] == $setup) {
      $user = $info[1];
      $task = $info[2];
      $dir = rtrim($info[3]);
    }
  }

  if (file_exists($dir."/steps/new") ||
      file_exists($dir."/steps/1")) {
    $topd = dir($dir."/steps");
    while (false !== ($run = $topd->read())) {
      if (preg_match('/^([0-9]+)$/',$run,$match)
          && $run>0
          && !file_exists("$dir/steps/$run/deleted.$run")) {
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
    if (file_exists($dir."/steps/new") ||
        file_exists($dir."/steps/$id")) {
      $stat = stat("$dir/steps/$id/parameter.$id");
    }
    else {
      $stat = stat("$dir/steps/parameter.$id");
    }
    $experiment[$id]->start = $stat[9];
  }

  reset($experiment);
  while (list($id,$info) = each($experiment)) {
    if (file_exists("$dir/evaluation/report.$id")) {
      $f = file("$dir/evaluation/report.$id");
      foreach ($f as $line_num => $line) {
	if (preg_match('/^(.+): (.+)/',$line,$match)) {
	  $experiment[$id]->result[$match[1]] = $match[2];
          if (!$evalset || !array_key_exists($match[1],$evalset)) {
            $evalset[$match[1]] = 0;
          }
	  $evalset[$match[1]]++;
	}
      }
    }
  }

  krsort($experiment);
  uksort($evalset,"evalsetsort");
}

function evalsetsort($a,$b) {
  if ($a == "avg") { return -1; }
  if ($b == "avg") { return 1; }
  return strcmp($a,$b);
}

function load_parameter($run) {
  global $dir;
  if (file_exists($dir."/steps/new") ||
      file_exists($dir."/steps/$run")) {
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
//    $comment[$item[0]]->url = $item[2];
//    $comment[$item[0]]->description = $item[3];
  }
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

	if (!$experiment || !array_key_exists($run,$experiment) ||
            !property_exists($experiment[$run],"last_step_time") ||
	    $time > $experiment[$run]->last_step_time) {
	  $experiment[$run]->last_step_time = $time;
	  $experiment[$run]->last_step = $step;
	}
    }
  }
}

function get_analysis_version($dir,$set,$id) {
  global $analysis_version;
  if ($analysis_version
      && array_key_exists($id,$analysis_version)
      && array_key_exists($set,$analysis_version[$id])) {
    #reset($analysis_version[$id][$set]);
    #print "$id,$set ( ";
    #while(list($type,$i) = each($analysis_version[$id][$set])) {
    #  print "$type=$i ";
    #}
    #print ") FROM CACHE<br>";
    return $analysis_version[$id][$set];
  }
  $analysis_version[$id][$set]["basic"] = 0;
  $analysis_version[$id][$set]["biconcor"] = 0;
  $analysis_version[$id][$set]["coverage"] = 0;
  $analysis_version[$id][$set]["precision"] = 0;
  $prefix = "$dir/evaluation/$set.analysis";

  # produced by the run itself ?
  if (file_exists("$prefix.$id/summary")) {
    $analysis_version[$id][$set]["basic"] = $id;
  }
  if (file_exists("$prefix.$id/input-annotation")) {
    $analysis_version[$id][$set]["coverage"] = $id;
  }
  if (file_exists("$prefix.$id/precision-by-input-word")) {
    $analysis_version[$id][$set]["precision"] = $id;
  }
  if (file_exists("$dir/model/biconcor.$id")) {
    $analysis_version[$id][$set]["biconcor"] = $id;
  }

  # re-use ?
  if (file_exists("$dir/steps/$id/re-use.$id")) {
    $re_use = file("$dir/steps/$id/re-use.$id");
    foreach($re_use as $line) {
      if (preg_match("/EVALUATION:(.+):analysis (\d+)/",$line,$match) &&
	  $match[1] == $set &&
	  file_exists("$prefix.$match[2]/summary")) {
	$analysis_version[$id][$set]["basic"] = $match[2];
      }
      else if (preg_match("/EVALUATION:(.+):analysis-coverage (\d+)/",$line,$match) &&
	  $match[1] == $set &&
	  file_exists("$prefix.$match[2]/input-annotation")) {
	$analysis_version[$id][$set]["coverage"] = $match[2];
      }
      else if (preg_match("/EVALUATION:(.+):analysis-precision (\d+)/",$line,$match) &&
	  $match[1] == $set &&
	  file_exists("$prefix.$match[2]/precision-by-input-word")) {
	$analysis_version[$id][$set]["precision"] = $match[2];
      }
      else if (preg_match("/TRAINING:build-biconcor (\d+)/",$line,$match) &&
        file_exists("$dir/model/biconcor.$match[1]")) {
	$analysis_version[$id][$set]["biconcor"] = $match[1];
      }
    }
  }

  # legacy stuff below...
  if (file_exists("$dir/steps/$id/REPORTING_report.$id")) {
   $report = file("$dir/steps/$id/REPORTING_report.$id.INFO");
   foreach ($report as $line) {
    if (preg_match("/\# reuse run (\d+) for EVALUATION:(.+):analysis$/",$line,$match) &&
        $match[2] == $set) {
      if (file_exists("$prefix.$match[1]/summary")) {
        $analysis_version[$id][$set]["basic"] = $match[1];
      }
    }
    if (preg_match("/\# reuse run (\d+) for EVALUATION:(.+):analysis-coverage/",$line,$match) &&
        $match[2] == $set) {
      if (file_exists("$prefix.$match[1]/input-annotation")) {
        $analysis_version[$id][$set]["coverage"] = $match[1];
      }
    }
    if (preg_match("/\# reuse run (\d+) for EVALUATION:(.+):analysis-precision/",$line,$match) &&
        $match[2] == $set) {
      if (file_exists("$prefix.$match[1]/precision-by-input-word")) {
        $analysis_version[$id][$set]["precision"] = $match[1];
      }
    }
    if (preg_match("/\# reuse run (\d+) for TRAINING:biconcor/",$line,$match)){
      if (file_exists("$dir/model/biconcor.$match[1]")) {
	$analysis_version[$id][$set]["biconcor"] = $match[1];
      }
    }
   }
  }
  #print "$id,$set ( ";
  #reset($analysis_version[$id][$set]);
  #while(list($type,$i) = each($analysis_version[$id][$set])) {
  #    print "$type=$i ";
  #}
  #print ") ZZ<br>";
  return $analysis_version[$id][$set];
}

function get_precision_analysis_version($dir,$set,$id) {
  $version = get_analysis_version($dir,$set,$id);
  return $version["precision"];
}

function get_basic_analysis_version($dir,$set,$id) {
  $version = get_analysis_version($dir,$set,$id);
  return $version["basic"];
}

function get_coverage_analysis_version($dir,$set,$id) {
  $version = get_analysis_version($dir,$set,$id);
  return $version["coverage"];
}

function get_biconcor_version($dir,$set,$id) {
  $version = get_analysis_version($dir,$set,$id);
  return $version["biconcor"];
}

function get_analysis_filename($dir,$set,$id,$type,$file) {
  $version = get_analysis_version($dir,$set,$id);
  return "$dir/evaluation/$set.analysis.".$version[$type]."/".$file;
}

function get_current_analysis_filename($type,$file) {
  global $dir,$set,$id;
  $version = get_analysis_version($dir,$set,$id);
  return "$dir/evaluation/$set.analysis.".$version[$type]."/".$file;
}

function get_current_analysis_filename2($type,$file) {
  global $dir,$set,$id2;
  $version = get_analysis_version($dir,$set,$id2);
  return "$dir/evaluation/$set.analysis.".$version[$type]."/".$file;
}
