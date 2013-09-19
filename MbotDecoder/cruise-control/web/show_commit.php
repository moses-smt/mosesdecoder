<?php

include("html_templates.php");
include("log_wrapper.php");

$type = $_GET["type"];
$commit_id = $_GET["commit_id"];

if (($type != "log" && $type != "info") || empty($commit_id)) {
  warn("Wrong arguments, dying") && die();
}

header("Content-Type: text/plain; charset=UTF-8");

include(StaticData::logs_path . "/" . substr($commit_id, 0, 1) . "/" . $commit_id . "." . $type);

?>
