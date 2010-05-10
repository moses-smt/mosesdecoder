<?php

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
    if (file_exists($dir."/steps/new") ||
        file_exists($dir."/steps/1")) {
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
  ksort($evalset);
}

function load_parameter($run) {
  global $dir;
  if (file_exists($dir."/steps/new") ||
      file_exists($dir."/steps/1")) {
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
