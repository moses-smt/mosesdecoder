<?php

function show_header($title)
{
  echo "
<html>
  <head>
    <META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;charset=utf-8\">
    <title>$title</title>
  </head><body>";
} 

function show_heading($text, $size = 1)
{
  echo "
    <h$size>$text</h$size>";
}

function show_footer()
{
  echo "
  </body>
<html>";
}

function end_table()
{
  echo "
    </table>";
}

function array_to_table_row($odd = true, $data)
{
  $bgcolor = $odd ? " bgcolor=\"#ccccdd\"" : "";
  echo "
    <tr$bgcolor>";
  foreach ($data as &$item) {
    echo "
      <td style=\"padding-left:8px; padding-right:8px\">$item</td>";
  }
  echo "
    </tr>";
}

function start_table()
{
  echo '
    <table rules="cols" frame="vsides">';
}

function start_form($action, $method = "get")
{
  echo "
    <form action=\"$action\" method=\"$method\">";
}

function end_form()
{
  echo "
    </form>";
}

function show_select_box($items, $name, $selected = "", $onchange_hdl = "")
{
  $onchange = $onchange_hdl ? " onchange=\"$onchange_hdl\"" : "";
  echo "
    <select name=\"$name\"$onchange>";
  foreach ($items as &$item) {
    $item_selected = $selected == $item ? " selected=\"yes\"" : "";
    echo "
      <option value=\"$item\"$item_selected>$item</option>";
  }
  echo "
    </select>";
}

function get_href($label, $url, $new_window = false)
{
  $target = $new_window ? " target=\"_blank\"" : "";
  return "<a href=\"$url\"$target>$label</a>";
}

function warn($msg)
{
  echo "<p><font color=\"red\"><b>$msg</b></font>";
}

function get_current_url()
{
  return $_SERVER["REQUEST_URI"];
}

function set_var($url, $var, $value)
{
  $url = cut_var($url, $var);
  if ($url[strlen($url) - 1] == "?") {
    $url .= "$var=$value";
  } elseif (strpos($url, "?") !== false) {
    $url .= "&$var=$value";
  } else {
    $url .= "?$var=$value";
  }
  return $url;
}

function cut_var($url, $var)
{
  // XXX there is probably a cleaner solution for this
  return preg_replace('/&?' . $var . '=[^&]+/', '', $url);
}

?>
