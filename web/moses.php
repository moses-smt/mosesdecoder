<html>
	<head><title>Moses demo</title></head>
<body>
	<A HREF="../">back <<--</A><BR><BR>

	<B>Moses demo</B><BR><BR>
<?php 

	$strInput = "";
	$strOutput= "";

	if ($_SERVER['REQUEST_METHOD'] == 'POST')
	{
		$input			= $_REQUEST['txtInput'];
		$inputLower	= strtolower($input);

		$inputFile = fopen('input', 'a') or die("can't open input file");
		$outputFile = fopen('output', 'r') or die("can't open output file");

		fwrite($inputFile, $inputLower ."\n");

		$output = fgets($outputFile);

		fclose($inputFile);
		fclose($outputFile);
	}
?>

<BR>
<form action="moses.php" method="POST">
<textarea name="txtInput" rows="5" cols="50"><?=$input?></textarea>
<BR>
<input type="submit" name="txt_submit" value="Submit">
</form><br><br>

<?php
	if ($_SERVER['REQUEST_METHOD'] == 'POST')
	{
		echo "Input sentence is: ".$inputLower."<BR>"; 
		echo "Translated is: " .$output ."<BR>";
	}
?>

<H6>
Copyright 2007 <A HREF="http://www.factored-translation.com/">Factored Translation</A><BR>
Licensed under the <A HREF="http://www.gnu.org/licenses/lgpl.html">LGPL</A><BR>
</H6>
</body>
</html>
