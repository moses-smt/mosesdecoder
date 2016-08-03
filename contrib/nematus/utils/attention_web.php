<!DOCTYPE html>
<html lang="en">
<head>
  <meta name="description" content="The HTML5 Herald">
  <meta name="author" content="SitePoint">
<title>Visualization Demo</title>
    <link href="_assets/css/global.css" type="text/css" rel="stylesheet">
    <link href="_assets/css/fonts.css" rel='stylesheet' type='text/css'>

    <link href="_assets/css/basic.css" type="text/css" rel="stylesheet" />
    <link href="_assets/css/visualize.css" type="text/css" rel="stylesheet" />
    <link href="_assets/css/visualize-light.css" type="text/css" rel="stylesheet" />
    <script type="text/javascript" src="_assets/js/d3.min.js"  charset="utf-8"></script>
    <!--script type="text/javascript" src="_assets/js/jquery.min.js"></script-->
    <script src="_assets/js/jquery-1.9.1.js"></script>
    <script src="_assets/js/jquery.min.js"></script>
    <!--script src="http://malsup.github.com/jquery.form.js"></script-->
    <script type="text/javascript" src="_assets/js/jquery.validate.min.js"></script>
    <!--script type="text/javascript" src="jquery-validation-1.11.1/lib/jquery.js"></script-->
    <script type="text/javascript" src="_assets/js/visualize.jQuery.js"></script>
    <script type="text/javascript" src="_assets/js/plugins.js" ></script>
    <script type="text/javascript" src="_assets/js/global.js" ></script>
    <!--link rel="stylesheet" type="text/css" href="_assets/css/jquery.mobile-1.4.5.min.css"/>
    <script src="_assets/js/jquery.mobile-1.4.5.min.js"></script!-->
    <script language="javascript" src="_assets/calendar/calendar/calendar.js"></script>
    <script type="text/javascript">
/*      $(function() {
        var page = '';
        if (page) "
          $('li.' + page).addClass('active');
        }
      });*/
    </script>
  <!--[if lt IE 9]>
    <script src="http://html5shiv.googlecode.com/svn/trunk/html5.js"></script>
  <![endif]-->
</head>
<style>
body{
	width:1200px;
	margin:100px auto;
}
svg text{
	font-size:12px;
}
rect{
	shape-rendering:crispEdges;
}


</style>
<body>
<div id="area1"></div>
<div id="area2"></div>
<script src="http://d3js.org/d3.v3.min.js"></script>
<script src="attention.js"></script>
<script>
var target = [["eine", "republi@@", "kanische", "Strategie", ",", "um", "der", "Wiederwahl", "von", "Obama", "entgegenzutreten", "EOS"]];//[['katze','bin','eine','katze'],['a','cat']];
var source = [["a", "Republican", "strategy", "to", "counter", "the", "re-election", "of", "Obama", "EOS"]];//[['cat','am','a','cat'],['eine','katze']]
var sales_data=[
/*['abc','def',0.16,0,0],
['abc','def',0.78,0,0],
['abc','df4',0.10,0,0],
['abc','df3',0.58,0,0],
['abc','df2',0.99,0,0],
['abc1','df1',0.45,1]*/
/*[0,0,0.96,0,0],
[0,1,0.23,0,0],
[0,2,0.20,0,0],
[0,3,0.66,0,0],
[1,0,0.23,0,0],
[1,1,0.88,0,0],
[1,2,0.34,0,0],
[1,3,0.39,0,0],
[2,0,0.20,0,0],
[2,1,0.55,0,0],
[2,2,0.94,0,0],
//[2,2,0.78,0,0],
[2,3,0.30,0,0],
[3,0,0.67,0,0],
[3,1,0.34,0,0],
[3,2,0.55,0,0],
[3,2,0.44,0,0],
[3,3,0.8,0,0],
[0,0,0.95,1,1],
[0,1,0.55,1,1],
[1,0,0.55,1,1],
[1,1,0.95,1,1]*/
[0, 0, 0.8606763482093811, 0, 0], [0, 0, 0.07394544780254364, 1, 0], [0, 0, 0.0050795478746294975, 2, 0], [0, 0, 0.014749017544090748, 3, 0], [0, 0, 0.00689421221613884, 4, 0], [0, 0, 0.007193631026893854, 5, 0], [0, 0, 0.0023485629353672266, 6, 0], [0, 0, 0.002437103074043989, 7, 0], [0, 0, 0.0041503868997097015, 8, 0], [0, 0, 0.022525690495967865, 9, 0], [1, 0, 0.008867422118782997, 0, 0], [1, 0, 0.9746508002281189, 1, 0], [1, 0, 0.006591449491679668, 2, 0], [1, 0, 0.0015248449053615332, 3, 0], [1, 0, 0.0019079294288530946, 4, 0], [1, 0, 0.0003202259249519557, 5, 0], [1, 0, 0.0014177649281919003, 6, 0], [1, 0, 0.00010574129555607215, 7, 0], [1, 0, 6.681956438114867e-05, 8, 0], [1, 0, 0.0045470003969967365, 9, 0], [2, 0, 0.017237717285752296, 0, 0], [2, 0, 0.13048002123832703, 1, 0], [2, 0, 0.005057570990175009, 2, 0], [2, 0, 0.0023649714421480894, 3, 0], [2, 0, 0.0011181416921317577, 4, 0], [2, 0, 0.0006632875301875174, 5, 0], [2, 0, 0.0031986164394766092, 6, 0], [2, 0, 0.002395403804257512, 7, 0], [2, 0, 0.0023285525385290384, 8, 0], [2, 0, 0.835155725479126, 9, 0], [3, 0, 0.0007024086662568152, 0, 0], [3, 0, 0.004170551430433989, 1, 0], [3, 0, 0.986028254032135, 2, 0], [3, 0, 0.0025388309732079506, 3, 0], [3, 0, 0.0006586579256691039, 4, 0], [3, 0, 5.288098327582702e-05, 5, 0], [3, 0, 0.0005594021058641374, 6, 0], [3, 0, 0.00013853979180566967, 7, 0], [3, 0, 2.5045808797585778e-05, 8, 0], [3, 0, 0.005125435534864664, 9, 0], [4, 0, 0.004196085501462221, 0, 0], [4, 0, 0.005216502584517002, 1, 0], [4, 0, 0.01276960875838995, 2, 0], [4, 0, 0.539452850818634, 3, 0], [4, 0, 0.12216465175151825, 4, 0], [4, 0, 0.042928386479616165, 5, 0], [4, 0, 0.0028886948712170124, 6, 0], [4, 0, 0.019435355439782143, 7, 0], [4, 0, 0.008831476792693138, 8, 0], [4, 0, 0.24211636185646057, 9, 0], [5, 0, 0.0017269871896132827, 0, 0], [5, 0, 0.0004039984487462789, 1, 0], [5, 0, 0.0005107998149469495, 2, 0], [5, 0, 0.07915166765451431, 3, 0], [5, 0, 0.2411874532699585, 4, 0], [5, 0, 0.5567825436592102, 5, 0], [5, 0, 0.004832962993532419, 6, 0], [5, 0, 0.00825989805161953, 7, 0], [5, 0, 0.03283322602510452, 8, 0], [5, 0, 0.07431045174598694, 9, 0], [6, 0, 0.0011403380194678903, 0, 0], [6, 0, 0.00031833394314162433, 1, 0], [6, 0, 0.00016495760064572096, 2, 0], [6, 0, 0.013225900009274483, 3, 0], [6, 0, 0.24790987372398376, 4, 0], [6, 0, 0.6110280752182007, 5, 0], [6, 0, 0.016282757744193077, 6, 0], [6, 0, 0.011716436594724655, 7, 0], [6, 0, 0.05048535019159317, 8, 0], [6, 0, 0.04772792384028435, 9, 0], [7, 0, 0.00010706923058023676, 0, 0], [7, 0, 0.0017059684032574296, 1, 0], [7, 0, 0.0001661568967392668, 2, 0], [7, 0, 0.0003117379965260625, 3, 0], [7, 0, 0.018107030540704727, 4, 0], [7, 0, 0.027465928345918655, 5, 0], [7, 0, 0.9100472927093506, 6, 0], [7, 0, 0.009352157823741436, 7, 0], [7, 0, 0.012818872928619385, 8, 0], [7, 0, 0.019917771220207214, 9, 0], [8, 0, 0.0002763264928944409, 0, 0], [8, 0, 8.873225306160748e-05, 1, 0], [8, 0, 1.1112268111901358e-05, 2, 0], [8, 0, 5.050943946116604e-05, 3, 0], [8, 0, 0.0018735375488176942, 4, 0], [8, 0, 0.005560200195759535, 5, 0], [8, 0, 0.0037295932415872812, 6, 0], [8, 0, 0.13867616653442383, 7, 0], [8, 0, 0.8325313329696655, 8, 0], [8, 0, 0.017202477902173996, 9, 0], [9, 0, 0.00012520256859716028, 0, 0], [9, 0, 0.00010352569370297715, 1, 0], [9, 0, 1.2285055163374636e-05, 2, 0], [9, 0, 8.521879863110371e-06, 3, 0], [9, 0, 0.00027309719007462263, 4, 0], [9, 0, 0.0010494114831089973, 5, 0], [9, 0, 0.002524220384657383, 6, 0], [9, 0, 0.0026750960387289524, 7, 0], [9, 0, 0.9733797311782837, 8, 0], [9, 0, 0.019848907366394997, 9, 0], [10, 0, 0.0016987099079415202, 0, 0], [10, 0, 0.0014881069073453546, 1, 0], [10, 0, 0.004352489486336708, 2, 0], [10, 0, 0.007792209275066853, 3, 0], [10, 0, 0.8691822290420532, 4, 0], [10, 0, 0.003717853920534253, 5, 0], [10, 0, 0.006918297614902258, 6, 0], [10, 0, 0.006369971204549074, 7, 0], [10, 0, 0.0013700248673558235, 8, 0], [10, 0, 0.09711004048585892, 9, 0], [11, 0, 0.003006486687809229, 0, 0], [11, 0, 0.0015359335811808705, 1, 0], [11, 0, 0.003430589335039258, 2, 0], [11, 0, 0.047293197363615036, 3, 0], [11, 0, 0.03258756920695305, 4, 0], [11, 0, 0.005403296090662479, 5, 0], [11, 0, 0.0019564235117286444, 6, 0], [11, 0, 0.008543130941689014, 7, 0], [11, 0, 0.005399803165346384, 8, 0], [11, 0, 0.8908436298370361, 9, 0]
];
/*['Lite','AZ',5453,35],
['Small','AZ',683,1],
['Medium','AZ',862,0],
['Grand','AZ',6228,30],
['Lite','AL',15001,449],
['Small','AL',527,3],
['Medium','AL',836,0],
['Plus','AL',28648,1419],
['Grand','AL',3,0]
/*['Lite','CO',13,0],
['Small','CO',396,0],
['Medium','CO',362,0],
['Plus','CO',78,10],
['Grand','CO',2473,32],
['Elite','CO',2063,64],
['Medium','DE',203,0],
['Grand','DE',686,2],
['Elite','DE',826,0],
['Lite','KS',1738,110],
['Small','KS',12925,13],
['Medium','KS',15413,0],
['Small','GA',2166,2],
['Medium','GA',86,0],
['Plus','GA',348,3],
['Grand','GA',4244,18],
['Elite','GA',1536,1],
['Small','IA',351,0],
['Grand','IA',405,1],
['Small','IL',914,1],
['Medium','IL',127,0],
['Grand','IL',1470,7],
['Elite','IL',516,1],
['Lite','IN',43,0],
['Small','IN',667,1],
['Medium','IN',172,0],
['Plus','IN',149,1],
['Grand','IN',1380,5],
['Elite','IN',791,23],
['Small','FL',1,0],
['Grand','FL',1,0],
['Small','MD',1070,1],
['Grand','MD',1171,2],
['Elite','MD',33,0],
['Plus','TX',1,0],
['Small','MS',407,0],
['Medium','MS',3,0],
['Grand','MS',457,2],
['Elite','MS',20,0],
['Small','NC',557,0],
['Medium','NC',167,0],
['Plus','NC',95,1],
['Grand','NC',1090,5],
['Elite','NC',676,6],
['Lite','NM',1195,99],
['Small','NM',350,3],
['Medium','NM',212,0],
['Grand','NM',1509,8],
['Lite','NV',3899,389],
['Small','NV',147,0],
['Medium','NV',455,0],
['Plus','NV',1,1],
['Grand','NV',4100,16],
['Lite','OH',12,0],
['Small','OH',634,2],
['Medium','OH',749,0],
['Plus','OH',119,1],
['Grand','OH',3705,19],
['Elite','OH',3456,25],
['Small','PA',828,2],
['Medium','PA',288,0],
['Plus','PA',141,0],
['Grand','PA',2625,7],
['Elite','PA',1920,10],
['Small','SC',1146,2],
['Medium','SC',212,0],
['Plus','SC',223,4],
['Grand','SC',1803,6],
['Elite','SC',761,8],
['Small','TN',527,0],
['Medium','TN',90,0],
['Grand','TN',930,4],
['Elite','TN',395,1],
['Lite','ME',7232,58],
['Small','ME',1272,0],
['Medium','ME',1896,0],
['Plus','ME',1,0],
['Grand','ME',10782,33],
['Elite','ME',1911,3],
['Small','VA',495,0],
['Medium','VA',32,0],
['Plus','VA',7,0],
['Grand','VA',1557,12],
['Elite','VA',24,0],
['Small','WA',460,1],
['Plus','WA',88,3],
['Grand','WA',956,3],
['Small','WV',232,0],
['Medium','WV',71,0],
['Grand','WV',575,2],
['Elite','WV',368,3]*/

var width = 2200, height = 690, margin ={b:0, t:40, l:-50, r:50};
var c = "area1";
var svg = d3.select("#area1")
//var svg  = d3.select(c).select("div").select(".plots")
	.append("svg").attr('width',width/2).attr('height',(height+margin.b+margin.t)/2)
	.append("g").attr("transform","translate("+ margin.l+","+margin.t+")");

var data = [ 
	{data:bP.partData(sales_data,0,0,target,source), id:'SalesAttempts', header:["Channel","State", "Sales Attempts"]}
//	{data:bP.partData(sales_data,3), id:'Sales', header:["Channel","State", "Sales"]}
];

bP.draw(data, svg);
var data = [ 
        {data:bP.partData(sales_data,1,1,target,source), id:'SsAttempts', header:["Channel","State", "Sales Attempts"]}
//      {data:bP.partData(sales_data,3), id:'Sales', header:["Channel","State", "Sales"]}
];
svg2 = d3.select("#area2")
        .append("svg").attr('width',width/2).attr('height',(height+margin.b+margin.t)/2)
        .append("g").attr("transform","translate("+ margin.l+","+margin.t+")");
bP.draw(data, svg2);
</script>
</body>
