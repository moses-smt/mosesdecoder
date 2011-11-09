<?php

include("html_templates.php");
include("log_wrapper.php");

const SHOW_ITEMS = 50;
const GITHUB_LINK = "https://github.com/moses-smt/mosesdecoder/commit/";

show_header("Moses Cruise Control");
echo "\n<center>\n";

show_heading("Moses Cruise Control");
echo "\n</center>\n";

// show current status of 'master' branch
$master_branch = new Branch("master");
$last_commit = $master_branch->get_next_commit();
$last_commit->read_log();
show_heading("Current status of master: " . colorize_status($last_commit->get_status()), 3);
$branch_name = ! empty($_GET["branch"]) ? $_GET["branch"] : "master";

// check that user wants to see a valid branch
$all_branches = get_all_branch_names();
if (! in_array($branch_name, $all_branches)) {
  warn("Branch '$branch_name' not found (only branches with some tests done can be viewed)");
  $branch_name = "master";
}

// branch select box
start_form("", "get");
echo "<p>Showing log of branch: ";
show_select_box($all_branches, "branch", $branch_name, "submit()");
end_form();

$branch = new Branch("$branch_name");
$start_with = ! empty($_GET["start"]) ? $_GET["start"] : 0;
$branch->set_line($start_with);

show_navigation($start_with);

// table of commits
start_table();
array_to_table_row(true, array("<b>Commit Link</b>", "<b>Status</b>", "<b>Full Log</b>",
  "<b>Timestamp</b>", "<b>Author</b>", "<b>Commit Message</b>" ));
for ($i = 0; $i < SHOW_ITEMS; $i++) {
  $last_commit = $branch->get_next_commit();

  if ( $last_commit->get_name() == "" ) {
    array_to_table_row(array("=== End of log ==="));
    break;
  }
  $last_commit->read_log();
  $last_commit->read_info();

  array_to_table_row(($i % 2 == 1),
    array( get_href(substr($last_commit->get_name(), 0, 10) . "...", GITHUB_LINK . $last_commit->get_name(), true),
           colorize_status($last_commit->get_status()),
           $last_commit->was_tested() ? get_href("Log", $last_commit->get_log_file(), true) : "N/A",
           $last_commit->get_timestamp(),
           $last_commit->get_author(),
           substr($last_commit->get_message(), 0, 30) . (strlen($last_commit->get_message()) > 30 ? "..." : "")));
}

end_table();

show_navigation($start_with);
show_footer();

// HTML ends here

function colorize_status($status)
{
  switch ( substr(strtolower($status), 0, 1) ) {
    case "o":
      $color = "green";
      break;
    case "f":
      $color = "red";
      break;
    default:
      $color = "#FFDD00";
  }
  return "<font color=\"$color\"><b>$status</b></font>";
}

function show_navigation($start_with)
{
  start_form("", "get");
  if ($start_with > 0) {
    echo get_href("<p>Previous",
      set_var(get_current_url(), "start", max(0, $start_with - SHOW_ITEMS)));
  } else {
    echo "Previous";
  }
  echo " ";

  echo get_href("Next", set_var(get_current_url(), "start", $start_with + SHOW_ITEMS));
  end_form();
}

?>
