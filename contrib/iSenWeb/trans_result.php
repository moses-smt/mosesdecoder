<?php
        $result = "";	
	$Content = $_POST['input1'];
	$ereg='/\n/';
        $arr_str = preg_split($ereg,$Content);
	foreach($arr_str as $value){
		$result = ` echo $value | nc 161.64.89.129 1986`;
		echo $result.'<br>';
	}       
?>
