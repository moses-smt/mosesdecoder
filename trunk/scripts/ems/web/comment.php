<?php 
  $fp = fopen("comment","a");
  fwrite($fp,$_GET{'run'} . ";" . $_GET{'text'} . "\n");
  fclose($fp);
?>
