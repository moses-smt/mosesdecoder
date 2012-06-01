<?php

class StaticData
{
  const logs_path = "./data/";
}

function get_all_branch_names()
{
  $branches = array();
  $dir_hdl = opendir(StaticData::logs_path);
  while (($file = readdir($dir_hdl)) !== false) {
    if (($pos = strpos($file, ".revlist")) > 0) {
      array_push($branches, substr($file, 0, $pos));
    }
  }
  return $branches;
}

class Branch
{  
  public function __construct($name)
  {
    $this->name = $name;
    $this->create_revlist_hdl();
  }

  public function get_next_commit()
  {
    return new Commit( chop( fgets($this->revlist_hdl) ) );
  }

  public function set_line($line)
  {
    $this->reset();
    $index = 0;
    while ($this->revlist_hdl && $index < $line) {
      fgets($this->revlist_hdl);
      $index++;
    }
  }

  public function reset()
  {
    fclose($this->revlist_hdl);
    $this->create_revlist_hdl();
  }

  private function create_revlist_hdl()
  {
    $this->revlist_hdl = fopen(StaticData::logs_path . "/" . $this->name . ".revlist", "r");
  }

  private $name;
  private $revlist_hdl;
}

class Commit
{
  public function __construct($name)
  {
    $this->name = $name;
  }

  public function read_log()
  {
    if (! $this->was_tested()) {
      return;
    }

    $log_hdl = fopen(StaticData::logs_path . "/" . substr($this->name, 0, 1) . "/" . $this->name . ".log", "r");
    while (($line = fgets($log_hdl)) !== false) {
      if (preg_match('/tests passed/', $line)) {
        $this->passed_percent = substr($line, 0, strpos('%', $line));
      } 
      else if (preg_match('/INVESTIGATE THESE FAILED TESTS/', $line)) {
        $this->failed_tests = substr($line, 39);
      }
       else if (! $this->is_ok() && preg_match('/## Status:/', $line)) {
        $this->failed_at = substr($line, 16);
      }
    }
  }

  public function read_info()
  {
    $info_hdl = fopen(StaticData::logs_path . "/" . substr($this->name, 0, 1) . "/" . $this->name . ".info", "r");
    while (($line = fgets($info_hdl)) !== false) {
      if (preg_match('/Author:/', $line)) {
        $this->author = substr("$line", 7);
      }
      else if (preg_match('/Date:/', $line)) {
        $this->timestamp = substr("$line", 12);
        break;
      }
    }
    while (($line = fgets($info_hdl)) !== false) {
      $this->message .= chop($line);
    }
    $this->message = preg_replace('/\s+/', ' ', $this->message);
  }

  public function was_tested()
  {
    return file_exists(StaticData::logs_path . "/" . substr($this->name, 0, 1) . "/" . $this->name . ".log");
  }

  public function is_ok()
  {
    if (! $this->was_tested()) {
      return false;
    } else {
      return file_exists(StaticData::logs_path . "/" . substr($this->name, 0, 1) . "/" . $this->name . ".OK");
    }
  }
  
  public function get_status()
  {
    return $this->was_tested()
      ? ($this->is_ok() ? "OK" : "Failed: " . $this->get_failed_at()) : "Not tested";
  }

  public function get_passed_percent()
  {
    return $this->passed_percent;
  }

  public function get_failed_tests()
  {
    return $this->failed_tests;
  }

  public function get_failed_at()
  {
    return $this->failed_at;
  }

  public function get_message()
  {
    return $this->message;
  }

  public function get_author()
  {
    return $this->author;
  }

  public function get_timestamp()
  {
    return $this->timestamp;
  }

  public function get_name()
  {
    return $this->name;
  }

  public function get_log_file()
  {
    return "show_commit.php?commit_id=$this->name&type=log"; 
  }

  public function get_info_file()
  {
    return "show_commit.php?commit_id=$this->name&type=info"; 
  }

  private function open_log()
  {
    return fopen($this->get_log_file());
  }

  private function open_info()
  {
    return fopen($this->get_info_file());
  }

  private $name;
  private $passed_percent;
  private $failed_tests;
  private $failed_at;
  private $message;
  private $author;
  private $timestamp;
  
}

?>
