<?php

function diff() {
  global $experiment;
  $display = $_GET["run"];
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
  print "<H3>Experiment $change</H3><TABLE>";

  // get parameter values for the two runs
  $parameter_base = load_parameter($base);
  $parameter_change = load_parameter($change);

  // get parameters and sort them
  $all_parameters = array_keys($parameter_base);
  foreach (array_keys($parameter_change) as $parameter) {
    if (!array_key_exists($parameter,$parameter_base)) {
      $all_parameters[] = $parameter;
    }
  }
  sort($all_parameters);

  // display differences
  foreach ($all_parameters as $parameter) {
    if (!array_key_exists($parameter,$parameter_base)) {
      $parameter_base[$parameter] = "";
    } 
    if (!array_key_exists($parameter,$parameter_change)) {
      $parameter_change[$parameter] = "";
    }
    if ($parameter_base[$parameter] != $parameter_change[$parameter]) {
      output_diff_line($parameter,$parameter_base[$parameter],$parameter_change[$parameter]);
    }
  }
  print "</TABLE>\n";
}

function output_diff_line($parameter,$base_value,$change_value) {
  print "<TR><TD BGCOLOR=yellow>$parameter</TD><TD BGCOLOR=lightgreen>$change_value</TD></TR><TR><TD>&nbsp;</TD><TD BGCOLOR=#cccccc>$base_value</TD></TR>\n";
}
