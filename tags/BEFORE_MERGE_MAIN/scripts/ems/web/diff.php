<?php

function diff() {
  global $experiment;
  $display = $_GET[run];
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
  $parameter_base = load_parameter($base);
  $parameter_change = load_parameter($change);
  print "<H3>Experiment $change</H3><TABLE>";
  while (list($parameter,$base_value) = each($parameter_base)) {
    if ($base_value != $parameter_change[$parameter]) {
      output_diff_line($parameter,$base_value,$parameter_change[$parameter]);
    }
  }
  while (list($parameter,$change_value) = each($parameter_change)) {
    if (!$parameter_base[$parameter]) {
      output_diff_line($parameter,"",$change_value);
    }
  }
  print "</TABLE>\n";
}

function output_diff_line($parameter,$base_value,$change_value) {
  print "<TR><TD BGCOLOR=yellow>$parameter</TD><TD BGCOLOR=lightgreen>$change_value</TD></TR><TR><TD>&nbsp;</TD><TD BGCOLOR=#cccccc>$base_value</TD></TR>\n";
}
