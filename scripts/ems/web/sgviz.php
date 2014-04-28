<?php
function sgviz($sentence) {
  global $setup,$dir,$id,$set;
?><html><head><title>Search Graph Visualization, Sentence <?php $sentence ?></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<script language="javascript" src="javascripts/prototype.js"></script></head>
<body><svg id="sg" height="500" width="900" xmlns="http://www.w3.org/2000/svg"><g id="chart"></g></svg>
<script>
var sg = document.getElementById("sg");
sg.setAttribute("width", window.innerWidth-20);
sg.setAttribute("height",window.innerHeight-20);
<?php
// read input sentence
$handle = fopen(get_current_analysis_filename("coverage","input-annotation"),"r");
for($i=0;$i<$sentence;$i++) { $line = fgets($handle); }
$line = fgets($handle);
fclose($handle);
$l = explode("\t",$line);
print "input=[\"<s>\",\"".join("\",\"",explode(" ",addslashes($l[0])))."\",\"</s>\"];\n";
?>
</script>
<script language="javascript" src="sgviz.js"></script>
<script>
var edge = new Array();
function test() {
  alert("test");
}
new Ajax.Request('?analysis=sgviz_data'
                 + '&setup=<?php print $setup ?>'
                 + '&id=<?php print $id ?>'
                 + '&set=<?php print $set ?>'
                 + '&sentence=<?php print $sentence; ?>',
  {
    onSuccess: function(transport) {
      var json = transport.responseText.evalJSON();
      edge = json.edge;
      process_hypotheses();
    },
    method: "post"
  });
</script></body></html>
<?php 
// read graph
//$file = get_current_analysis_filename("basic","search-graph")."/graph.$sentence";
//$handle = fopen($file,"r");
//while (($line = fgets($handle)) !== false) {
//  $e = explode("\t",addslashes(chop($line)));
//  print "edge[$e[0]]=new Array($e[1],$e[2],$e[3],\"$e[4]\",\"$e[5]\",\"$e[6]\",$e[7],$e[8],$e[9],\"$e[10]\");\n";
//}
//fclose($handle);
}

function sgviz_data($sentence) {
  header('Content-type: application/json');
  $file = get_current_analysis_filename("basic","search-graph")."/graph.$sentence";

  $handle = fopen($file,"r");
  while (($line = fgets($handle)) !== false) { 
    $e = explode("\t",addslashes(chop($line)));
    $edge[$e[0]] = array($e[1],$e[2],$e[3],$e[4],$e[5],$e[6],$e[7],$e[8],$e[9],$e[10]);
  }
  $return['edge'] = $edge;
  print json_encode($return);
  exit();
}
