// javascript util to visualize attention weights between source and target

!function(){
	var bP={};	
	var b=30, bb=150, height=600, buffMargin=1, minHeight=14;
	var c1=[-130, 40], c2=[-50, 100], c3=[-10, 140]; //Column positions of labels.
	var colors =["#3366CC", "#DC3912",  "#FF9900","#109618", "#990099", "#0099C6"];
	
	bP.partData = function(data,p,p1,target,source){
		var sData={};
	        console.log(sData);	
		sData.keys=[
                          source[p],target[p1]
			//d3.set(source.map(function(d){ if (d[0] == p) return d[0];})).values().sort(function(a,b){ return ( a<b? -1 : a>b ? 1 : 0);}),//values
			//d3.set(target.map(function(d){ if (d[1] == p1) return d[1];})).values().sort(function(a,b){ return ( a<b? -1 : a>b ? 1 : 0);})//keys		
		];
		console.log(sData);
		sData.data = [	sData.keys[0].map( function(d){ return sData.keys[1].map( function(v){ return 10; }); }),//source[p],
				sData.keys[1].map( function(d){ return sData.keys[1].map( function(v){ return 10; }); }),
				sData.keys[0].map( function(d){ return sData.keys[1].map( function(v){ return 0; });})//sData.keys[0].map( function(d){ return sData.keys[1].map( function(v){ return 0; }); }),
				//		sData.keys[1].map( function(d){ return sData.keys[0].map( function(v){ return 0; }) ;}),
				//		sData.keys[0].map( function(d){ return sData.keys[1].map( function(v){ return 0; });})  
		];
		/*console.log(sData);
                source[p].forEach(function(d){
			console.log("so " + d + " " +sData.keys[0].indexOf(d));
			sData.data[0][sData.keys[0].indexOf(d)][sData.keys[0].indexOf(d)]= 10;
                });*/
		console.log(sData.keys[0].map( function(d){ return sData.keys[1].map( function(v){ return 0; });}));
		data.forEach(function(d){ 
		//	console.log("sd0 " + sData.keys[1]);
			if((d[1] == p1) && (d[4] == p)){ //((d[3] == p) && (d[4] == p1)){
				console.log("DGG" + d[2] + " t1 " + d[3]);
				//sData.data[0][d[0]][d[0]]=10;//[sData.keys[0].indexOf(d[0])][sData.keys[0].indexOf(d[0])]=10;//[sData.keys[1].indexOf(d[1])]=10;//d[p];//target
				//sData.data[1][d[1]][d[0]]=10;//[sData.keys[1].indexOf(d[1])][sData.keys[1].indexOf(d[1])]=10;//[sData.keys[0].indexOf(d[0])]=10;//d[p]; //source
                        	sData.data[2][d[3]][d[0]] = d[2];//sData.data[2][d[1]][d[0]]=d[2];//[sData.keys[0].indexOf(d[0])][sData.keys[1].indexOf(d[1])]=d[p];//edges
			}
		});
	        console.log(sData);
		return sData;
	}
	
	function visualize(data){
		var vis ={};
		function calculatePosition(a, s, e, b, m){
			var total=d3.sum(a);//a :data
			var sum=0, neededHeight=0, leftoverHeight= e-s-2*b*a.length;
			var ret =[];
		        console.log(a);	
			a.forEach(
				function(d){ 
					var v={};
                                        //d=30;
					v.percent = (total == 0 ? 0 : d/total); 
					v.value=d;
					v.height=Math.max(v.percent*(e-s-2*b*a.length), m);
					(v.height==m ? leftoverHeight-=m : neededHeight+=v.height );
					ret.push(v);
				}
			);
			
			var scaleFact=leftoverHeight/Math.max(neededHeight,1), sum=0;
			
			ret.forEach(
				function(d){ 
					d.percent = scaleFact*d.percent; 
					d.height=(d.height==m? m : d.height*scaleFact);
					d.middle=sum+b+d.height/2;
					d.y=s + d.middle - d.percent*(e-s-2*b*a.length)/2;
					d.h= d.percent*(e-s-2*b*a.length);
					d.percent = (total == 0 ? 0 : d.value/total);
					sum+=2*b+d.height;
				}
			);
			return ret;
		}
                function calculatePosition1(a, s, e, b, m,d1){
                        var total=d3.sum(a);//a :data
                        var sum=0, neededHeight=0, leftoverHeight= e-s-2*b*a.length;
                        var ret =[];
                        console.log(a);
                        a.forEach(
                                function(d){
                                        var v={};
                                        //d=30;
                                        v.percent = (total == 0 ? 0 : d/total);
                                        v.value=d;
                                        v.height=Math.max(v.percent*(e-s-2*b*a.length), m);
                                        (v.height==m ? leftoverHeight-=m : neededHeight+=v.height );
                                        ret.push(v);
                                }
                        );

                        var scaleFact=leftoverHeight/Math.max(neededHeight,1), sum=0;

                        ret.forEach(
                                function(d){
                                        d.percent = scaleFact*d.percent;
                                        d.height=(d.height==m? m : d.height*scaleFact);
                                        d.middle=sum+b+d.height/2;
                                        d.y=s + d.middle - d.percent*(e-s-2*b*a.length)/2;
                                        d.h= d.percent*(e-s-2*b*a.length);
                                        d.percent = (total == 0 ? 0 : d.value/total);
                                        sum+=2*b+d.height;
					d.p = d1;
                                }
                        );
                        return ret;
                }
		//console.log(" 1 " + data.data[0] + " 2 " + data.data[1].map(function(d){ return d3.sum(d);}));
		vis.mainBars = [ 
			//console.log(" 1 " + data.data[0].map(function(d){ return d3.sum(d);}));// + " 2 " + data.data[1].map(function(d){ return d3.sum(d);}));
			calculatePosition1( data.data[0].map(function(d){ return d3.sum(d);}), 0, height, buffMargin, minHeight,0),
			calculatePosition1( data.data[1].map(function(d){ return d3.sum(d);}), 0, height, buffMargin, minHeight,1)
		];
		console.log("ma ",vis.mainBars);
		vis.subBars = [[],[]];
		vis.edges = [];
		vis.mainBars[0].forEach(function(pos,i){
			 vis.mainBars[1].forEach(function(bar, j){
				sBar = [];
				//vis.edges = [];
				//console.log("k1" + data.keys[0][i] + " k2 " +data.keys[1][j] + " d" +data.data[2][i][j] + " ba "+bar.y);
                                sBar.key1 = data.keys[0][j];//[i];
				sBar.key2 = data.keys[1][i];//[j];
				sBar.y1 = pos.middle;
				sBar.y2 = bar.middle;
				sBar.h1 =1;//bar.y;
				sBar.h2 = 180;
				sBar.va = data.data[2][i][j];
				vis.edges.push(sBar);	
				
				/*calculatePosition(data.data[p][i], bar.y, bar.y+bar.h, 0, 0).forEach(function(sBar,j){ 
					sBar.key1=i;//(p==0 ? i : j); 
					sBar.key2=j;//(p==0 ? j : i); 
                                        sBar.p = p;
					vis.subBars[p].push(sBar); 
				});*/
			});
		});
		console.log(vis.edges);
		/*vis.subBars.forEach(function(sBar){
			sBar.sort(function(a,b){ 
				return (a.key1 < b.key1 ? -1 : a.key1 > b.key1 ? 
						1 : a.key2 < b.key2 ? -1 : a.key2 > b.key2 ? 1: 0 )});
		});*/
	       //console.log(vis.subBars);	
		/*vis.edges = vis.mainBars[0].map(function(p,i){
                        //console.log("k1" + p.key1 + " k2 " +p.key2 + " val " + data.data[2][0][0] + " ph " +p.h + " py " + p.y  + " ph " + vis.subBars[0][0].middle + " py " + vis.subBars[1][i].middle);
			return {
				key1: p.key1,//vis.subBars[1][i].key1,
				key2: p.key2,//vis.subBars[1][i].key2,
				y1:vis.subBars[1][0].y,
				y2:p.y,//middle,//vis.subBars[1][i].h,
				h2:vis.subBars[1][0].y,
				h1:180,//p.y// + 120//vis.subBars[1][i].y + 150
				va:data.data[2][0][i]
			};
		});*/
		vis.keys=data.keys;
		return vis;
	}
	
	function arcTween(a) {
		var i = d3.interpolate(this._current, a);
		this._current = i(0);
		return function(t) {
			return edgePolygon(i(t));
		};
	}
	
	function drawPart(data, id, p){
		d3.select("#"+id).append("g").attr("class","part"+p)
			.attr("transform","translate( 0," +( p*(bb+b))+")"); //+( p*(bb+b))+",0)");
		d3.select("#"+id).select(".part"+p).append("g").attr("class","subbars");
		d3.select("#"+id).select(".part"+p).append("g").attr("class","mainbars");
		
		/*var mainbar = d3.select("#"+id).select(".part"+p).select(".mainbars")
			.selectAll(".mainbar").data(data.mainBars[p])
			.enter().append("g").attr("class","mainbar");

		mainbar.append("rect").attr("class","mainrect")
			.attr("x", 0).attr("y",function(d){ return d.middle-d.height/2; })
			.attr("width",b).attr("height",function(d){ return d.height; })
			.style("shape-rendering","auto")
			.style("fill-opacity",0).style("stroke-width","0.5")
			.style("stroke","black").style("stroke-opacity",0);
			
		mainbar.append("text").attr("class","barlabel")
			.attr("x", c1[p]).attr("y",function(d){ return d.middle+5;})
			.text(function(d,i){ return data.keys[p][i];})
			.attr("text-anchor","start" );
			
		mainbar.append("text").attr("class","barvalue")
			.attr("x", c2[p]).attr("y",function(d){ return d.middle+5;})
			.text(function(d,i){ return d.value ;})
			.attr("text-anchor","end");
			
		mainbar.append("text").attr("class","barpercent")
			.attr("x", c3[p]).attr("y",function(d){ return d.middle+5;})
			.text(function(d,i){ return "( "+Math.round(100*d.percent)+"%)" ;})
			.attr("text-anchor","end").style("fill","grey");*/
	        var me = "0em";
		var bar = d3.select("#"+id).select(".part"+p).select(".mainbars")
                        .selectAll(".mainbar").data(data.mainBars[p])
                        .enter();//d3.select("#"+id).select(".part"+p).select(".subbars")
			//.selectAll(".subbar").data(data.subBars[p]).enter();
			
		     bar.append("rect").attr("class","mainbar")//"subbar")
			.attr("x", function(d){ return d.middle})
                        .attr("y",p)
			.attr("width",function(d){ return d.h})
			.attr("height",b)
			.style("fill","None");//function(d){ if (d.p == 1) {return colors[d.key2];} else {return colors[d.key1];}});
			/*.attr("x", 0).attr("y",function(d){ return d.y})
			.attr("width",b).attr("height",function(d){ return d.h})
			.style("fill",function(d){ return colors[d.key1];});*/
		bar.append("text")
        	  .attr("x", function(d) {
     	                return d.middle;
                    })
        //.attr("y", 11)
        .attr("dy", function(d){ 
                        if (d.p == 1) {
			    return "2em";
			} else {
			    return "0em";
			}
	})//"1.35em")
        .attr("dx", "2em")
        .text(function(d,i) { return data.keys[p][i];
                       /* if (d.p == 1) {
                            var a = data.keys[1];
                            console.log(data.keys[1] + " n " + a[1] + " ff " +d.key2);
                            return a[d.key2];// + "%";
                        } else {
			    var a = data.keys[0];
                            return a[d.key1];
                        }*/
	})
        .attr("text-anchor", "left")
	.attr("fill","black");
	}
	
	function drawEdges(data, id){
		var color = d3.interpolateLab("#FBCEB1","#E34234");
		d3.select("#"+id).append("g").attr("class","edges").attr("transform","translate("+ b+",0)");

		d3.select("#"+id).select(".edges").selectAll(".edge")
                         .data(data.edges).enter().append("line").attr("class","edge")
			.attr("x1", function(d) {return d.y1;})     // x position of the first end of the line
                        .attr("y1", function(d) {return d.h1;})      // y position of the first end of the line
                        .attr("x2", function(d) {return d.y2;})     // x position of the second end of the line
                        .attr("y2", function(d) {return d.h2;})
   //.style("fill",function(d){ return colors[d.value];})
                        .style("stroke-width", function(d) { console.log(d); return d.va * 2; })//function(d) { console.log(d); return d.value * 2; })
                        .style("stroke", function(d) {return color(d.va);});
			/*.data(data.edges).enter().append("polygon").attr("class","edge")
			.attr("points", edgePolygon).style("fill",function(d){ return colors[d.key1];})
			.style("opacity",0.5).each(function(d) { this._current = d; });	*/
	}	
	
	function drawHeader(header, id){
		d3.select("#"+id).append("g").attr("class","header").append("text").text(header[2])
			.style("font-size","20").attr("x",108).attr("y",-20).style("text-anchor","middle")
			.style("font-weight","bold");
		
		[0,1].forEach(function(d){
			var h = d3.select("#"+id).select(".part"+d).append("g").attr("class","header");
			
			h.append("text").text(header[d]).attr("x", (c1[d]-5))
				.attr("y", -5).style("fill","grey");
			
			h.append("text").text("Count").attr("x", (c2[d]-10))
				.attr("y", -5).style("fill","grey");
			
			h.append("line").attr("x1",c1[d]-10).attr("y1", -2)
				.attr("x2",c3[d]+10).attr("y2", -2).style("stroke","black")
				.style("stroke-width","1").style("shape-rendering","crispEdges");
		});
	}
	
	function edgePolygon(d){
		return [0, d.y1, bb, d.y2, bb, d.y2+d.h2, 0, d.y1+d.h1].join(" ");
	}	
	
	function transitionPart(data, id, p){
		var mainbar = d3.select("#"+id).select(".part"+p).select(".mainbars")
			.selectAll(".mainbar").data(data.mainBars[p]);
		
		mainbar.select(".mainrect").transition().duration(500)
			.attr("y",function(d){ return d.middle-d.height/2;})
			.attr("height",function(d){ return d.height;});
			
		mainbar.select(".barlabel").transition().duration(500)
			.attr("y",function(d){ return d.middle+5;});
			
		mainbar.select(".barvalue").transition().duration(500)
			.attr("y",function(d){ return d.middle+5;}).text(function(d,i){ return d.value ;});
			
		mainbar.select(".barpercent").transition().duration(500)
			.attr("y",function(d){ return d.middle+5;})
			.text(function(d,i){ return "( "+Math.round(100*d.percent)+"%)" ;});
			
		d3.select("#"+id).select(".part"+p).select(".subbars")
			.selectAll(".subbar").data(data.subBars[p])
			.transition().duration(500)
			.attr("y",function(d){ return d.y}).attr("height",function(d){ return d.h});
	}
	
	function transitionEdges(data, id){
		d3.select("#"+id).append("g").attr("class","edges")
			.attr("transform","translate("+ b+",0)");

		d3.select("#"+id).select(".edges").selectAll(".edge").data(data.edges)
			.transition().duration(500)
			.attrTween("points", arcTween)
			.style("opacity",function(d){ return (d.h1 ==0 || d.h2 == 0 ? 0 : 0.5);});	
	}
	
	function transition(data, id){
		transitionPart(data, id, 0);
		transitionPart(data, id, 1);
		transitionEdges(data, id);
	}
	
	bP.draw = function(data, svg){
		data.forEach(function(biP,s){
			svg.append("g")
				.attr("id", biP.id)
				.attr("transform","translate( 0, " + (100)+")");//+ (550*s)+",0)");
				
			var visData = visualize(biP.data);
			drawPart(visData, biP.id, 0);
			drawPart(visData, biP.id, 1); 
			drawEdges(visData, biP.id);
		//	drawHeader(biP.header, biP.id);
			
		/*	[0,1].forEach(function(p){			
				d3.select("#"+biP.id)
					.select(".part"+p)
					.select(".mainbars")
					.selectAll(".mainbar")
					.on("mouseover",function(d, i){ return bP.selectSegment(data, p, i); })
					.on("mouseout",function(d, i){ return bP.deSelectSegment(data, p, i); });	
			});*/
		});	
	}
	
	bP.selectSegment = function(data, m, s){
		data.forEach(function(k){
			var newdata =  {keys:[], data:[]};	
				
			newdata.keys = k.data.keys.map( function(d){ return d;});
			
			newdata.data[m] = k.data.data[m].map( function(d){ return d;});
			
			newdata.data[1-m] = k.data.data[1-m]
				.map( function(v){ return v.map(function(d, i){ return (s==i ? d : 0);}); });
			
			transition(visualize(newdata), k.id);
				
			var selectedBar = d3.select("#"+k.id).select(".part"+m).select(".mainbars")
				.selectAll(".mainbar").filter(function(d,i){ return (i==s);});
			
			selectedBar.select(".mainrect").style("stroke-opacity",1);			
			selectedBar.select(".barlabel").style('font-weight','bold');
			selectedBar.select(".barvalue").style('font-weight','bold');
			selectedBar.select(".barpercent").style('font-weight','bold');
		});
	}	
	
	bP.deSelectSegment = function(data, m, s){
		data.forEach(function(k){
			transition(visualize(k.data), k.id);
			
			var selectedBar = d3.select("#"+k.id).select(".part"+m).select(".mainbars")
				.selectAll(".mainbar").filter(function(d,i){ return (i==s);});
			
			selectedBar.select(".mainrect").style("stroke-opacity",0);			
			selectedBar.select(".barlabel").style('font-weight','normal');
			selectedBar.select(".barvalue").style('font-weight','normal');
			selectedBar.select(".barpercent").style('font-weight','normal');
		});		
	}
	
	this.bP = bP;
}();
