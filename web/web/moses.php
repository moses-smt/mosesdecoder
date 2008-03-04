<html>
	<head><title>Moses demo</title></head>
<body>
	<B>
	Moses demo<BR><BR>
<?php 

	$strInput = "";
	$strOutput= "";

	if ($_SERVER['REQUEST_METHOD'] == 'POST')
	{
		$strInput		= $_REQUEST['txt'];
		echo "Input is: ".$strInput."<BR>"; 

		$inputFile = fopen('input', 'a') or die("can't open input file");
		$outputFile = fopen('output', 'r') or die("can't open output file");

		fwrite($inputFile, $strInput."\n");

		$strOutput = fgets($outputFile);

		fclose($inputFile);
		fclose($outputFile);
	}
?>

Output is: <?=$strOutput?><BR>
<BR>
<form action="moses.php" method="POST">
<textarea name="txt" rows="5" cols="50"><?=$strInput?></textarea>
<BR>
<input type="submit" name="txt_submit" value="Submit">
</form><br><br>


</body>
</html>
