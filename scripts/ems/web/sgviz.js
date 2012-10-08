var xmlns="http://www.w3.org/2000/svg";
var RECOMBINED = 0;
var FROM = 1;
var TO = 2;
var OUTPUT = 3;
var ALIGNMENT = 4;
var CHILDREN = 5;
var RULE_SCORE = 6;
var HEURISTIC_RULE_SCORE = 7;
var HYP_SCORE = 8;
var LHS = 9;
var DERIVATION_SCORE = 10;
var CHART_WIDTH  = window.innerWidth * 0.8;
var CHART_HEIGHT = window.innerHeight;
var CELL_WIDTH = CHART_WIDTH/input.length;
var CELL_HEIGHT = CHART_HEIGHT/(input.length+1);
var CELL_MARGIN = 4;
var CELL_BORDER = 2;
var CELL_PADDING = 2;
if (input.length < 6) { CELL_MARGIN = 5; CELL_BORDER = 3; CELL_PADDING = 3; }
if (input.length > 10) { CELL_MARGIN = 1; CELL_BORDER = 1; CELL_PADDING = 2; }
if (input.length > 20) { CELL_MARGIN = 0; CELL_BORDER = 0; CELL_PADDING = 1; }
var BUTTON_WIDTH = 170;
var BUTTON_HEIGHT = 30;
var OPTION_WIDTH = 60;
var OPTION_HEIGHT = BUTTON_HEIGHT;
var CELL_HIGHLIGHT_COLOR = "#c0ffc0";
var CELL_REGULAR_COLOR = "#ffff80";
var INPUT_HIGHLIGHT_COLOR = "#c0c0c0";
var INPUT_REGULAR_COLOR = "#ffffff";
var SORT_OPTION = 2;
var ZOOM = 0;
var ZOOM_FROM = 0;
var ZOOM_TO = input.length+1;
var ZOOM_WIDTH = input.length;

var length = input.length;
var chart = document.getElementById("chart");
var reachable = new Array();
var cell_hyps = new Array(length);
var cell_derivation_score = Array();

// init basic layout
draw_chart();
draw_menu();
draw_options();

// process hypotheses
function process_hypotheses() {
  index_hypotheses_by_cell();
  find_reachable_hypotheses();
  compute_best_derivation_scores();
}

//
// INITIALIZATION
// 

function index_hypotheses_by_cell() {
  // init edge_lists
  for(var from=0; from<length; from++) {
    cell_hyps[from] = new Array(length);
    for(var to=0; to<length; to++) {
     	cell_hyps[from][to] = new Array();
    }
  }
  // populate
  for (var id in edge) {
    var from = edge[id][FROM];
    var to = edge[id][TO];
    edge[id][FROM] = parseInt(from);
    edge[id][TO] = parseInt(to);
    edge[id][RULE_SCORE] = parseFloat(edge[id][RULE_SCORE]);
    edge[id][HEURISTIC_RULE_SCORE] = parseFloat(edge[id][HEURISTIC_RULE_SCORE]);
    edge[id][HYP_SCORE] = parseFloat(edge[id][HYP_SCORE]);
    edge[id][DERIVATION_SCORE] = parseFloat(edge[id][DERIVATION_SCORE]);
    cell_hyps[from][to].push(id);
  }
}

function find_reachable_hypotheses() {
  for (var i=0;i<cell_hyps[0][length-1].length;i++) {
    id = cell_hyps[0][length-1][i];
    find_reachable_hypotheses_recursive( id );
  }
}

function find_reachable_hypotheses_recursive( id ) {
  if (!(reachable[id] === undefined)) { return; }
  reachable[id] = 1;
  var children = get_children( id );
  for(var c=0;c<children.length;c++) {
    find_reachable_hypotheses_recursive( children[c] );
  }
}
  
function compute_best_derivation_scores() {
  for(var from=0; from<length; from++ ) {
    cell_derivation_score[from] = Array();
  }
  for(var width=length-1; width>=0; width-- ) {
    for(var from=0; from<length-width; from++ ) {
      var to = from+width;
      var cell_max_score = -9.9e9;
      for (var i=0;i<cell_hyps[from][to].length;i++) {
        var id = cell_hyps[from][to][i];
        if (width == length-1) {
          edge[id][DERIVATION_SCORE] = edge[id][HYP_SCORE];
        }
        if (edge[id][DERIVATION_SCORE] != null) {
          var children = get_children(id);
          for(var c=0;c<children.length;c++) {
            if (edge[children[c]][DERIVATION_SCORE] == null ||
                edge[children[c]][DERIVATION_SCORE] < edge[id][DERIVATION_SCORE]) {
              edge[children[c]][DERIVATION_SCORE] = edge[id][DERIVATION_SCORE];
            }
          }
        }
        if (edge[id][DERIVATION_SCORE] != null &&
            edge[id][DERIVATION_SCORE] > cell_max_score) {
          cell_max_score = edge[id][DERIVATION_SCORE];
        }
      }
      cell_derivation_score[from][to] = cell_max_score;
    }
  }
}

//
// MENU
//

function draw_menu() {
  draw_menu_button(1,"Best Derivation");
  draw_menu_button(2,"Number of Hypotheses");
  draw_menu_button(3,"Number of Rule Cubes");
  draw_menu_button(4,"Derivation Score");
  draw_menu_button(5,"Non-Terminals")
  draw_menu_button(6,"Hypotheses")
}
var MENU_POSITION_HYPOTHESES = 6; // where is "Hypotheses" in the menu?

var current_menu_selection = 0;
var menu_processing = 0;
function click_menu( id, force_flag ) {
  if (!force_flag && (menu_processing || current_menu_selection == id)) {
    return;
  }
  menu_processing = 1;
  
  if (current_menu_selection == 1) { best_derivation(0); }
  if (current_menu_selection == 2) { unannotate_cells(); }
  if (current_menu_selection == 3) { unannotate_cells(); }
  if (current_menu_selection == 4) { unannotate_cells(); }
  if (current_menu_selection == 5) { remove_non_terminal_treemap(0); }
  if (current_menu_selection == 6 && SORT_OPTION != 3) { remove_hypothesis_overview(); }
  if (current_menu_selection == 6 && SORT_OPTION == 3) { remove_hypothesis_overview(); remove_non_terminal_treemap(); }
  if (current_menu_selection > 0) { 
    highlight_menu_button( current_menu_selection, 0 );
  }

  if (id == 1) { best_derivation(1); }
  if (id == 2) { annotate_cells_with_hypcount(); }
  if (id == 3) { annotate_cells_with_rulecount(); }
  if (id == 4) { annotate_cells_with_derivation_score(); }
  if (id == 5) { non_terminal_treemap(); }
  if (id == 6 && SORT_OPTION != 3) { hypothesis_overview(); }
  if (id == 6 && SORT_OPTION == 3) { draw_hypothesis_sort_buttons(); non_terminal_treemap(1); }
  highlight_menu_button( id, 1 );
  current_menu_selection = id;
  menu_processing = 0;
}

function draw_menu_button( id, label ) {
  var button = document.createElementNS(xmlns,"rect");
  button.setAttribute("id", "button-" + id);
  button.setAttribute("x", 5);
  button.setAttribute("y", 5 + BUTTON_HEIGHT*(id-1));
  button.setAttribute("rx", 3);
  button.setAttribute("ry", 3);
  button.setAttribute("width", BUTTON_WIDTH-10);
  button.setAttribute("height", BUTTON_HEIGHT-10);
  //button.setAttribute("opacity",.75);
  button.setAttribute("fill", "#c0c0ff");
  button.setAttribute("stroke", "black");
  button.setAttribute("stroke-width", "1");
  button.setAttribute("onclick","click_menu(" + id + ",0);")
  chart.appendChild( button );

  var button_label = document.createElementNS(xmlns,"text");
  button_label.setAttribute("x", BUTTON_WIDTH/2);
  button_label.setAttribute("y", 4+BUTTON_HEIGHT/2 + BUTTON_HEIGHT*(id-1));
  button_label.setAttribute("style", "font-size: 12; font-family: Verdana, Arial;");
  button_label.setAttribute("text-anchor", "middle");
  button_label.setAttribute("pointer-events", "none");
  var content = document.createTextNode( label );
  button_label.appendChild( content );
  button_label.setAttribute("onclick","click_menu(" + id + ",0);")
  
  chart.appendChild( button_label );  
}

function highlight_menu_button( id, on_off ) {
  var button = document.getElementById("button-" + id);
  if (on_off) {
    button.setAttribute("fill", "#8080ff");
  }
  else {
    button.setAttribute("fill", "#c0c0ff");
  }
}

// OPTIONS

function draw_options() {
  draw_option_button(0,1,"score");
  draw_option_button(0,2,"deriv.");
  draw_option_button(0,3,"id");
}

function draw_rule_options() {
  draw_option_button(1,1,"score");
  draw_option_button(1,2,"deriv.");
  draw_option_button(1,3,"zoom");
  highlight_option_button(1,1,show_hyp_score);
  highlight_option_button(1,2,show_derivation_score);
}

function draw_option_button( rule_option, id, label ) {
  var button = document.createElementNS(xmlns,"rect");
  button.setAttribute("id", (rule_option?"rule-":"") + "option-" + id);
  button.setAttribute("x", rule_option ? CHART_WIDTH-BUTTON_WIDTH-OPTION_WIDTH : BUTTON_WIDTH+10);
  button.setAttribute("y", 5 + OPTION_HEIGHT*(id-1));
  button.setAttribute("rx", 3);
  button.setAttribute("ry", 3);
  button.setAttribute("width", OPTION_WIDTH-10);
  button.setAttribute("height", OPTION_HEIGHT-10);
  button.setAttribute("fill", "#fdd017");
  button.setAttribute("stroke", "black");
  button.setAttribute("stroke-width", "1");
  button.setAttribute("onclick","click_"+(rule_option?"rule_":"")+"option(" + id + ");")
  chart.appendChild( button );

  var button_label = document.createElementNS(xmlns,"text");
  var distance_from_side = BUTTON_WIDTH+5+OPTION_WIDTH/2;
  button_label.setAttribute("id", (rule_option?"rule-":"") + "option-label-" + id);
  button_label.setAttribute("x", rule_option ? CHART_WIDTH-distance_from_side : distance_from_side);
  button_label.setAttribute("y", 4+OPTION_HEIGHT/2 + OPTION_HEIGHT*(id-1));
  button_label.setAttribute("style", "font-size: 12; font-family: Verdana, Arial;");
  button_label.setAttribute("text-anchor", "middle");
  button_label.setAttribute("pointer-events", "none");
  var content = document.createTextNode( label );
  button_label.appendChild( content );
  
  chart.appendChild( button_label );  
}

function draw_sort_button( id, label ) {
  var BASE_X = 5 + id/SORT_BUTTON_COUNT * (BUTTON_WIDTH-10+5);
  var BASE_Y = -5 + BUTTON_HEIGHT * MENU_POSITION_HYPOTHESES;
  var WIDTH = ((BUTTON_WIDTH-10+5)/SORT_BUTTON_COUNT)-5;

  var button = document.createElementNS(xmlns,"rect");

  button.setAttribute("id", "sort-" + id);
  button.setAttribute("x", BASE_X);
  button.setAttribute("y", BASE_Y);
  button.setAttribute("width", WIDTH);
  button.setAttribute("height", BUTTON_HEIGHT-12);
  if (id==0) {
    button.setAttribute("fill", "none");
  }
  else {
    button.setAttribute("rx", 3);
    button.setAttribute("ry", 3);
    if (SORT_OPTION == id) {
      button.setAttribute("fill", "#6080ff");
    }
    else {
      button.setAttribute("fill", "#a0c0ff");
    }
    button.setAttribute("onclick","click_sort(" + id + ");")
    button.setAttribute("stroke", "black");
    button.setAttribute("stroke-width", "1");
  }
  chart.appendChild( button );

  var button_label = document.createElementNS(xmlns,"text");
  button_label.setAttribute("id", "sort-label-" + id);
  button_label.setAttribute("x", BASE_X + WIDTH/2);
  button_label.setAttribute("y", BASE_Y + 12);
  button_label.setAttribute("style", "font-size: 10; font-family: Verdana, Arial;");
  button_label.setAttribute("text-anchor", "middle");
  button_label.setAttribute("pointer-events", "none");
  var content = document.createTextNode( label );
  button_label.appendChild( content );
  
  chart.appendChild( button_label );  
}

function click_sort( id ) {
  if (SORT_OPTION == 3) {
    remove_non_terminal_treemap(1)
  }
  remove_hypothesis_overview();

  SORT_OPTION = id;

  if (SORT_OPTION == 3) {
    non_terminal_treemap(1);
    draw_hypothesis_sort_buttons();
  }
  else {
    hypothesis_overview();
  }
}

var show_scores = 0;
var show_id = 0;
var show_derivation = 0;
function click_option( id ) {
  if (id == 1) { 
    show_scores = !show_scores; 
    highlight_option_button( 0, 1, show_scores );
  }
  if (id == 2) { 
    show_derivation = !show_derivation; 
    color_cells();
    highlight_option_button( 0, 2, show_derivation );
  }
  if (id == 3) { 
    show_id = !show_id; 
    highlight_option_button( 0, 3, show_id );
  }
  if (current_menu_selection > 0) {
    click_menu( current_menu_selection, 1 );
  }
}

var show_hyp_score = 0;
var show_derivation_score = 0;
function click_rule_option( id ) {
  if (id == 1) { 
    show_hyp_score = !show_hyp_score; 
    highlight_option_button( 1, 1, show_hyp_score );
  }
  if (id == 2) { 
    show_derivation_score = !show_derivation_score; 
    highlight_option_button( 1, 2, show_derivation_score );
  }
  if (id == 3) { 
    if (ZOOM > 0) {
      ZOOM = 0;
    }
    else {
      ZOOM = 0.3;
    }
    assign_chart_coordinates();
    highlight_option_button( 1, 3, ZOOM );
  }
  draw_rule_cube(current_z_pos_string);
}

function highlight_option_button( rule_option, id, on_off ) {
  var button = document.getElementById((rule_option?"rule-":"") + "option-" + id);
  if (on_off) {
    button.setAttribute("fill", "#cd853f");
  }
  else {
    button.setAttribute("fill", "#fdd017");
  }
}

// INITIALIZE THE CHART

function draw_chart() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;
      
      // logical container
      var container = document.createElementNS(xmlns,"svg");
      container.setAttribute("id", "cell-container-" + from + "-" + to);
      chart.appendChild( container );
      var transform = document.createElementNS(xmlns,"g");
      transform.setAttribute("id", "cell-" + from + "-" + to);
      container.appendChild( transform );
    
      // yellow box for the cell
	    var cell = document.createElementNS(xmlns,"rect");
	    cell.setAttribute("id", "cellbox-" + from + "-" + to);
	    cell.setAttribute("x", CELL_MARGIN);
	    cell.setAttribute("y", CELL_MARGIN);
		  cell.setAttribute("rx", CELL_BORDER);
		  cell.setAttribute("ry", CELL_BORDER);
		  cell.setAttribute("width", CELL_WIDTH-2*CELL_MARGIN);
		  cell.setAttribute("height", CELL_HEIGHT-2*CELL_MARGIN);
		  cell.setAttribute("opacity", .75);
		  cell.setAttribute("fill", get_cell_color(from,to));
		  cell.setAttribute("stroke", "black");
		  cell.setAttribute("stroke-width", "1");
		  cell.setAttribute("onmouseover","hover_cell(" + from + "," + to + ");")
		  cell.setAttribute("onclick","click_cell(" + from + "," + to + ");")
		  transform.appendChild( cell );
    }
    
    // box for the input word
    var input_box = document.createElementNS(xmlns,"rect");
    input_box.setAttribute("id", "inputbox-" + from);
    input_box.setAttribute("x", CELL_MARGIN+from*CELL_WIDTH);
    input_box.setAttribute("y", CELL_MARGIN+(length)*CELL_HEIGHT);
	  input_box.setAttribute("rx", 3);
	  input_box.setAttribute("ry", 3);
	  input_box.setAttribute("width", CELL_WIDTH-2*CELL_MARGIN);
	  input_box.setAttribute("height", CELL_HEIGHT/2);
	  //cell.setAttribute("opacity", .75);
	  input_box.setAttribute("fill", INPUT_REGULAR_COLOR);
    chart.appendChild( input_box ); 
    
    // input word
    input_word = document.createElementNS(xmlns,"text");
    input_word.setAttribute("id", "input-" + from);
    input_word.setAttribute("x", (from+0.5)*CELL_WIDTH);
    input_word.setAttribute("y", 10+(length+0.25)*CELL_HEIGHT);
    input_word.setAttribute("style", "font-size:18;");
    input_word.setAttribute("text-anchor", "middle");
    var content = document.createTextNode( input[from] );
    input_word.appendChild( content );
    chart.appendChild( input_word );  
	}
  assign_chart_coordinates();
}

function assign_chart_coordinates() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;
      
	    var x = from*CELL_WIDTH + (width-1)*CELL_WIDTH/2;
      var y = (length-width)*CELL_HEIGHT*(1-ZOOM);
      //alert("(x,y) = (" + length + "," + width + "), width = " + ZOOM + ", height = " + (1-ZOOM));
      var cell_width = CELL_WIDTH;
      var cell_height = CELL_HEIGHT;
      if (ZOOM > 0) {
        // adjust (x,y)
        if (width > ZOOM_WIDTH) { // above
        }
        else if (width == ZOOM_WIDTH) { // same level
          x = from*CELL_WIDTH*(1-ZOOM) + (width-1)*CELL_WIDTH*(1-ZOOM)/2
          if (from < ZOOM_FROM) { // left
            y += (CHART_HEIGHT-2*CELL_HEIGHT)*ZOOM/2;
          }
          else if (from == ZOOM_FROM) { // the focus
          }
          else { // right
            x += CHART_WIDTH*ZOOM;
            y += (CHART_HEIGHT-2*CELL_HEIGHT)*ZOOM/2;
          }
        }
        else { // below
          y += CHART_HEIGHT*ZOOM-CELL_HEIGHT*(1-ZOOM);
        }

        // adjust width and height
        if (width == ZOOM_WIDTH) { // same level
          cell_width *= 1-ZOOM;
          if (from < ZOOM_FROM) { // left
          }
          else if (from == ZOOM_FROM) { // the focus
            cell_width += CHART_WIDTH*ZOOM;
            cell_height = CHART_HEIGHT*ZOOM;
          }
        }
        else {
          cell_height *= 1-ZOOM;
        }
      }

      var container = document.getElementById("cell-container-" + from + "-" + to);
      container.setAttribute("x", x);
	    container.setAttribute("y", y);
      var transform = document.getElementById("cell-" + from + "-" + to);
      transform.setAttribute("transform", "scale(" + (cell_width/CELL_WIDTH) + "," + (cell_height/CELL_HEIGHT) + ")");
    }
	}
}

function remove_chart() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;
      var container = document.getElementById("cell-" + from + "-" + to);
      chart.removeChild(container);
      var cell = document.getElementById("cellbox-" + from + "-" + to);
      chart.removeChild(cell);
    }
    var input_word = document.getElementById("input-" + from);
    chart.removeChild(input_word);
    var input_box = document.getElementById("inputbox-" + from);
    chart.removeChild(input_box);
  }
}

var current_from = -1;
var current_to;
function hover_cell( from, to ) {
  if (current_from >= 0) {
    highlight_input( current_from, current_to, 0)    
  }
  highlight_input( from, to, 1)
  current_from = from;
  current_to = to;
}

function click_cell( from, to ) {
  if (from == current_rule_from && to == current_rule_to) {
    unshow_rules();
    current_rule_from = -1;
    ZOOM = 0;
  }
  else {
    show_rules( from, to );
    ZOOM_FROM = current_rule_from;
    ZOOM_TO = current_rule_to;
    ZOOM_WIDTH = to-from+1;
  }
  assign_chart_coordinates();
}

function highlight_input( from, to, on_off ) {
  for(var i=from; i<=to; i++) {
    var input_box = document.getElementById("inputbox-" + i);
	  input_box.setAttribute("fill", on_off ? INPUT_HIGHLIGHT_COLOR : INPUT_REGULAR_COLOR);
  }
}

// 
// VISUALIZATION OF CHART CELLS
//

// BASIC ANNOTATION WITH NUMBERS

function annotate_cells_with_hypcount() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;
      annotate_cell( from, to, cell_hyps[from][to].length, 20 )
    }
	}
}

function annotate_cells_with_rulecount() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;
      var rule_hash = Array();
      var rule_count = 0;
      for (var i=0;i<cell_hyps[from][to].length;i++) {
        var rule = get_rule( cell_hyps[from][to][i] );
        if (rule_hash[rule] === undefined) {
          rule_hash[rule] = 1;
          rule_count++;
        }
      }
      annotate_cell( from, to, rule_count, 20 )
    }
	}
}

function annotate_cells_with_derivation_score() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;      
      var score = cell_derivation_score[from][to];
      if (score < -9e9) { score = "dead end"; }
      annotate_cell( from, to, score, 15 )
    }
	}
}

function annotate_cell( from, to, label, font_size ) {
  var cell_label_group = document.createElementNS(xmlns,"svg");
  cell_label_group.setAttribute("id", "celllabel-" + from + "-" + to);
  cell_label_group.setAttribute("x", 0);
  cell_label_group.setAttribute("y", -3);
  cell_label_group.setAttribute("pointer-events", "none");

  label = "" + label; // make it a string, if not already
  var line = label.split("<br>");
  for(var i=0; i<line.length; i++) {
    var cell_label = document.createElementNS(xmlns,"text");
    cell_label.setAttribute("id", "celllabelline-" + from + "-" + to + "-" + i);
    cell_label.setAttribute("x", CELL_WIDTH/2);
    cell_label.setAttribute("y", CELL_HEIGHT/2 + font_size * (1 - line.length/2 + i));
    cell_label.setAttribute("style", "font-size: " + font_size + ";font-family: Verdana, Arial;");
    cell_label.setAttribute("pointer-events", "none");
    cell_label.setAttribute("text-anchor", "middle");
    var content = document.createTextNode(line[i]);
    cell_label.appendChild( content );  
    cell_label_group.appendChild( cell_label );
  }

  var cell = document.getElementById("cell-" + from + "-" + to);
  cell.appendChild( cell_label_group );  
}

function unannotate_cells() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;
      unannotate_cell( from, to );
    }
	}
}

function unannotate_cell( from, to ) {
  var cell = document.getElementById("cell-" + from + "-" + to);
  var cell_label = document.getElementById("celllabel-" + from + "-" + to);
  cell.removeChild(cell_label);
}

// NON-TERMINAL TREEMAP

function non_terminal_treemap( with_hyps ) {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;      
      // get nt counts
      var lhs = new Array();
      var lhs_list = new Array();
      for (var i=0;i<cell_hyps[from][to].length;i++) {
        var id = cell_hyps[from][to][i];
        var nt = edge[id][LHS];
        if (lhs[nt] === undefined) {
          lhs[nt] = 1;
          lhs_list.push(nt);
        }
        else {
          lhs[nt]++;
        }
      }
      // sort 
      function sortByCount(a,b) {
        return lhs[b] - lhs[a];
      }
      lhs_list.sort(sortByCount);
      treemap_squarify( from, to, lhs_list, lhs, cell_hyps[from][to].length, with_hyps );
    }
  }
}

function remove_non_terminal_treemap()  {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;      
      var cell = document.getElementById("cell-" + from + "-" + to);
      var done = false;
      var j=0;
      while(!done) {
        var rect = document.getElementById("rect-" + from + "-" + to + "-" + j);
        if (rect == null) {
          done = true;
        }
        else {
          cell.removeChild(rect);
          var rect_label = document.getElementById("rect-label-" + from + "-" + to + "-" + j);
          if (rect_label != null) {
            cell.removeChild(rect_label);
          }
        }
        j++;
      }
    }
  }
}

function treemap_cell( from, to, label, count, total, with_hyps ) {
  var cell = document.getElementById("cell-" + from + "-" + to);
  var x = CELL_MARGIN;
  var y = CELL_MARGIN;
  var width = CELL_WIDTH - 2*CELL_MARGIN;
  var height = CELL_HEIGHT - 2*CELL_MARGIN;
  // TO DO
  for (var i=0;i<label.length;i++) {
    var rect = document.createElementNS(xmlns,"rect");
    rect.setAttribute("id", "-" + id);
    rect.setAttribute("x", x);
    rect.setAttribute("y", y);
    rect.setAttribute("width", width * count[label[i]] / total);
    rect.setAttribute("height", height);
    rect.setAttribute("opacity",.75);
    rect.setAttribute("fill", "#c0c0ff");
    rect.setAttribute("stroke", "black");
    rect.setAttribute("stroke-width", "0.5");
    rect.setAttribute("onclick","click_menu(" + id + ",0);")
    cell.appendChild( rect );  
    x += width * count[label[i]] / total;
  }
}

function treemap_squarify( from, to, label, count, total, with_hyps ) {
  var cell = document.getElementById("cell-" + from + "-" + to);
  // conversion of counts to area
  var width = CELL_WIDTH - 2*CELL_MARGIN;
  var height = CELL_HEIGHT - 2*CELL_MARGIN;
  var area_factor = width*height / total;
  var scale_factor = Math.sqrt( area_factor );
  width /= scale_factor;
  height /= scale_factor;
  var offset_x = 0;
  var offset_y = 0;

  // main algorithm
  var extend = Math.min(width,height);
  var current_worst = squarify_worst( label, count, 0, 0, extend );
  var start = 0;
  for(var i=1; i<=label.length; i++) {
    // add to sequence or start new one?
    var next_worst = 0;
    if (i != label.length) {
      next_worst = squarify_worst( label, count, start, i, extend );
    }
    if (i != label.length && current_worst >= next_worst) {
      current_worst = next_worst; // ... and keep going
    }
    else {
      // compute rectangles...
      var sum = 0;
      for(var j=start; j<i; j++) {
        sum += count[label[j]];
      }
      var cum_x = 0;
      var cum_y = 0;
      var remaining_ratio = sum / ((width - offset_x) * (height - offset_y));
      var this_width  = (width  - offset_x) * remaining_ratio;
      var this_height = (height - offset_y) * remaining_ratio;
      var adding_on_left = height-offset_y < width-offset_x;
      for(var j=start; j<i; j++) {
        if (adding_on_left) { this_height = count[label[j]] / sum * (height - offset_y); }
        else                {  this_width = count[label[j]] / sum *  (width - offset_x); }
        // rectangle
        var rect = document.createElementNS(xmlns,"rect");
        rect.setAttribute("id", "rect-" + from + "-" + to + "-" + j);
        rect.setAttribute("x", CELL_MARGIN + (offset_x + cum_x) * scale_factor);
        rect.setAttribute("y", CELL_MARGIN + (offset_y + cum_y) * scale_factor);
        rect.setAttribute("width", this_width * scale_factor);
        rect.setAttribute("height", this_height * scale_factor);
        rect.setAttribute("fill-opacity",0);
        rect.setAttribute("pointer-events", "none");
        rect.setAttribute("stroke", "black");
        rect.setAttribute("stroke-width", "0.5");
        cell.appendChild( rect );  
        // hypotheses
        if (with_hyps) {
          var hyp_list = Array();
          for(var k=0; k<cell_hyps[from][to].length; k++) {
            var id = cell_hyps[from][to][k];
            var nt = edge[id][LHS];
            if (nt == label[j]) {
              hyp_list.push( id );
            }
          }
          hypothesis_in_rect( this_width * scale_factor - 2, 
                              this_height * scale_factor - 2, 
                              CELL_MARGIN + (offset_x + cum_x) * scale_factor + 1, 
                              CELL_MARGIN + (offset_y + cum_y) * scale_factor + 1, 
                              cell, hyp_list );
        }
        // label
        var font_size = Math.min( Math.round(this_width * scale_factor / label[j].length * 1.3),
                                  Math.round(this_height * scale_factor )); 
        if (font_size > 20) { font_size = 20; }
        if (font_size >= 3) {
          var rect_label = document.createElementNS(xmlns,"text");
          rect_label.setAttribute("id", "rect-label-" + from + "-" + to + "-" + j);
          rect_label.setAttribute("x", CELL_MARGIN + (offset_x + cum_x + this_width/2) * scale_factor);
          rect_label.setAttribute("y", CELL_MARGIN + (offset_y + cum_y + this_height/2) * scale_factor + font_size/2 -2);
          rect_label.setAttribute("style", "font-size: " + font_size + "; font-family: Verdana, Arial; font-weight:900;");
          rect_label.setAttribute("fill", "#00f");
          rect_label.setAttribute("opacity", .3);
          rect_label.setAttribute("text-anchor", "middle");
          rect_label.setAttribute("pointer-events", "none");
          var content = document.createTextNode( label[j] );
          rect_label.appendChild( content );
          cell.appendChild( rect_label );  
        }
        if (adding_on_left) { cum_y += this_height; }
        else                { cum_x += this_width; }
      }
      if (adding_on_left) { offset_x += this_width; }
      else                { offset_y += this_height; }

      // move to next sequence
      if (i != label.length) {
        start = i;
        extend = Math.min( width-offset_x, height-offset_y );
        current_worst = squarify_worst( label, count, i, i, extend );
      }
    }
  }
}

function squarify_worst( label, count, start, end, extend ) {
  var sum = 0;
  for(var i=start; i<=end; i++) {
    sum += count[label[i]];
  }
  var max_ratio = 0;
  for(var i=start; i<=end; i++) {
    var ratio = count[label[i]] * extend*extend /sum/sum;
    if (ratio < 1) { ratio = 1/ratio; }
    max_ratio = Math.max( ratio, max_ratio );
  }
  return max_ratio;
}

// HIGHLIGHT BEST DERIVATION

function best_derivation( on_off ) {
  var best_score = -9e9;
  var best_id = -1;
  for (var i=0;i<cell_hyps[0][length-1].length;i++) {
    id = cell_hyps[0][length-1][i];
    if (edge[id][HYP_SCORE] > best_score) {
      best_score = edge[id][HYP_SCORE];
      best_id = id;
    }
  }
  best_derivation_recurse( best_id, on_off, -1, -1, 0 );
}

function best_derivation_recurse( id, on_off, parent_from, parent_to, child_pos ) {  
  var from = edge[id][FROM];
  var to = edge[id][TO];

  // highlight cell and annotate with rule
  highlight_cell( from, to, on_off );
  if (on_off) {
    var annotation = "";
    if (show_id) { annotation += id + "<br>";  }
    annotation += edge[id][LHS] + "\u2192";
    annotation += edge[id][OUTPUT];
    if (show_scores) { annotation += "<br>" + edge[id][HYP_SCORE];  }
    annotate_cell( from, to, annotation, 10 );
  }
  else {
    unannotate_cell( from, to );
  }
  
  // highlight hyp
  highlight_hyp( id, on_off );
  
  // arrow to parent
  if (parent_from >= 0) {
    if (on_off) {
      make_arrow( id, parent_from, parent_to, from, to, 0, child_pos );
    }
    else {
      var arrow = document.getElementById("arrow-" + id);
      chart.removeChild(arrow);
    }
  }
  
  var child_order = Array();
  if (edge[id][ALIGNMENT] != "") {
    var alignment = edge[id][ALIGNMENT].split(" ");
    // sorting: array position is source nonterminal pos
    alignment.sort();
    // alignment target sympol pos -> source nonterminal pos
    var reversed_alignment = Array();
    for(var i=0; i<alignment.length; i++) {
      var source_target = alignment[i].split("-");
      reversed_alignment.push(source_target[1]+"-"+i);
    }
    // sorting by symbols pos: array position is nonterminal pos
    reversed_alignment.sort();
    // mapping child -> target nonterminal pos
    for(var i=0; i<reversed_alignment.length; i++) {
      var target_source = reversed_alignment[i].split("-");
      child_order[target_source[1]] = i;
    }
  }
  
  // recurse
  var covered = new Array;
  var children = get_children( id );
  for(var c=0;c<children.length;c++) {
    var child = children[c];
    for( var i=edge[child][FROM]; i<=edge[child][TO]; i++ ) {
      covered[i] = 1;
    } 
    best_derivation_recurse( child, on_off, from, to, children.length == 1 ? 0.5 : child_order[c]/(children.length-1.0) );
  }

  // arrows to words
  for( var i=from; i<=to; i++ ) {
    if (covered[i] === undefined) {
      if (on_off) {
        make_arrow( "word-" + i, from, to, i, i, 1, 0.5 );
      }
      else {
        var arrow = document.getElementById("arrow-word-" + i);
        chart.removeChild(arrow);        
      }
    }
  } 
}

function make_arrow( id, parent_from, parent_to, from, to, word_flag, position ) {
  var arrow = document.createElementNS(xmlns,"line");
  arrow.setAttribute("id", "arrow-" + id);

  var parent = get_cellbox_coordinates( parent_from, parent_to );
  arrow.setAttribute("x1", parent.x+(0.5+position)/2*CELL_WIDTH*parent.scale_x);
  arrow.setAttribute("y1", parent.y+(CELL_HEIGHT-CELL_MARGIN)*parent.scale_y);

  if (word_flag) {
    arrow.setAttribute("x2", (from+.5)*CELL_WIDTH);
    arrow.setAttribute("y2", CELL_MARGIN+length*CELL_HEIGHT);
  }
  else {
    var child = get_cellbox_coordinates( from, to );
    arrow.setAttribute("x2", child.x+child.scale_x*CELL_WIDTH/2);
    arrow.setAttribute("y2", child.y+child.scale_y*CELL_MARGIN+word_flag*(CELL_HEIGHT+5));
  }

  arrow.setAttribute("stroke", word_flag ? "#808080" : "#008000");
  arrow.setAttribute("stroke-width", "3");
  chart.appendChild( arrow );
}

function get_cellbox_coordinates(from, to) {
  var container = document.getElementById("cell-container-" + from + "-" + to);
  var transform = document.getElementById("cell-" + from + "-" + to);
  var scale = transform.getAttribute("transform").split(/[\(\),]/g);
  var scale_x = scale[1];
  var scale_y = scale[2];
  return { x: Math.round(container.getAttribute("x")),
           y: Math.round(container.getAttribute("y")),
           scale_x: scale[1],
           scale_y: scale[2]
         }
}

function highlight_cell( from, to, on_off ) {
  var cell = document.getElementById("cellbox-" + from + "-" + to);
  cell.setAttribute("fill", on_off ? CELL_HIGHLIGHT_COLOR : get_cell_color(from,to) );
}

function color_cells() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from+width-1;
      highlight_cell(from,to,0);
    }
	}
}

function get_cell_color( from, to ) {
  if (!show_derivation) {
    return CELL_REGULAR_COLOR;
  }
  var score_diff = cell_derivation_score[from][to] - cell_derivation_score[0][length-1];
  var dec = 128 - (score_diff*8);
  if (dec>255) { dec = 255; }
  var color = Math.round(dec).toString(16);
  return "#ffff"+color;
}

function get_children( id ) {
  if (edge[id][CHILDREN] == "") {
    return [];
  }
  return edge[id][CHILDREN].split(" ");
}

// OVERVIEW ALL HYPOTHESES
function hypothesis_overview() {
  for (var from=0;from<length;from++) {
    for(var width=1; width<=length-from; width++) {
      var to = from + width - 1;
      hypothesis_overview_cell( from, to );
    }
  }
  draw_hypothesis_sort_buttons();
}

function draw_hypothesis_sort_buttons() {
  // draw sort buttons
  draw_sort_button(0,"sort by");
  draw_sort_button(1,"id");
  draw_sort_button(2,"score");
  draw_sort_button(3,"lhs");
}
var SORT_BUTTON_COUNT = 4; // how many in total (incl. "sort by")?

function hypothesis_overview_cell( from, to ) {
  var width = CELL_WIDTH-2*(CELL_BORDER+CELL_MARGIN+CELL_PADDING);
  var height = CELL_HEIGHT-2*(CELL_BORDER+CELL_MARGIN+CELL_PADDING);
	var cell = document.getElementById("cell-" + from + "-" + to);
  hypothesis_in_rect( width, height, CELL_BORDER+CELL_MARGIN+CELL_PADDING, CELL_BORDER+CELL_MARGIN+CELL_PADDING, cell, cell_hyps[from][to] );
}

function hypothesis_in_rect( width, height, offset_x, offset_y, parent_element, hyp_list ) {

  // diameter, based on perfect fill
  var diameter = Math.sqrt( width * height / hyp_list.length );
  // if it does not fit the discrete objects, increase
  while( Math.floor( width/diameter ) * Math.floor( height/diameter ) < hyp_list.length ) {
    diameter = Math.max(  width / Math.ceil(  width/diameter + 0.0001 ),   // fitting one more in row
                         height / Math.ceil( height/diameter + 0.0001 ) ); // fitting one more in column
  }
  var row_size = Math.floor( width / diameter );
  var column_size = Math.floor( height / diameter );

  // sort hypotheses
  function sortByScore(a,b) {
    return edge[b][HYP_SCORE] - edge[a][HYP_SCORE];
  }
  function sortById(a,b) {
    return a-b;
  }
  if (SORT_OPTION == 1) {
    hyp_list.sort(sortById);
  }
  else {
    hyp_list.sort(sortByScore);
  }

  // draw hypothesis
  var x=0
  var y=0;
  var column = 0;

  for (var i=0; i<hyp_list.length;i++) {
    id = hyp_list[i];
        
    //alert("adding circle (" + (x + diameter/2) + "," + (y + diameter/2) + ") - " + (diameter/2) );
    var hyp = document.createElementNS(xmlns,"circle");
    hyp.setAttribute("id", "hyp-" + id);
    hyp.setAttribute("cx", x + diameter/2 + offset_x);
    hyp.setAttribute("cy", y + diameter/2 + offset_y);
    hyp.setAttribute("r", diameter/2);
	  hyp.setAttribute("fill", hyp_color(id, 0));
	  hyp.setAttribute("onmouseover","hover_hyp(" + id + ");")
	  hyp.setAttribute("onmouseout","unhover_hyp(" + id + ");")
	  parent_element.appendChild( hyp );
	  
	  x += diameter;
    if (++column >= row_size) {
      column = 0;
      y += diameter;
      x = 0;
    }
  }
}

function remove_hypothesis_overview() {
  for (var id in edge) {
	  var cell = document.getElementById("cell-" + edge[id][FROM] + "-" + edge[id][TO]);
 	  var hyp = document.getElementById("hyp-" + id);
    cell.removeChild(hyp);
  }
  // remove sort buttons
  for(var i=0; i<4; i++) {
    var old = document.getElementById("sort-" + i);
    chart.removeChild( old );
    var old = document.getElementById("sort-label-" + i);
    chart.removeChild( old );
  }
}

function hover_hyp( id ) {
  best_derivation_recurse( id, 1, -1, -1 );
}

function unhover_hyp( id ) {
  best_derivation_recurse( id, 0, -1, -1 );
}

function hover_rule_hyp( id ) {
  highlight_rule_hyp( id, 1 );
  if (current_menu_selection == 1) {
    best_derivation( 0 );
  }
  if (current_menu_selection <= 2) {
    best_derivation_recurse( id, 1, -1, -1 );
  }
}

function unhover_rule_hyp( id ) {
  highlight_rule_hyp( id, 0 );
  if (current_menu_selection <= 2) {
    best_derivation_recurse( id, 0, -1, -1 );
  }
  if (current_menu_selection == 1) {
    best_derivation( 1 );
  }
}

function highlight_hyp( id, on_off ) {
  var hyp = document.getElementById("hyp-" + id);
  if (hyp == null) { return; }
  hyp.setAttribute("fill", hyp_color(id, on_off));
}

function highlight_rule_hyp( id, on_off ) {
  var hyp = document.getElementById("rule-hyp-" + id);
  if (hyp == null) { return; }
  hyp.setAttribute("fill", rule_hyp_color(id, on_off));
}

function hyp_color( id, on_off ) {
  if (on_off) {
    var color = "#ff0000";
    if (edge[id][RECOMBINED]>0) { color = "#808080"; }
    else if (id in reachable) { color = "#00c000"; }    
    return color;
  }
  var color = "#ffc0c0";
  if (edge[id][RECOMBINED]>0) { color = "#c0c0c0"; }
  else if (id in reachable) { color = "#80ff80"; }
  return color;
}

// RULES

function get_rule( id ) {
  // get non-terminal labels
  if (edge[id] === undefined) { alert("unknown edge "+id); return ""; }
  var output = edge[id][OUTPUT].split(" ");
  var alignment = edge[id][ALIGNMENT].split(" ");
  alignment.sort();
  var nt_label = Array();
  for(var i=0;i<alignment.length;i++) {
    var source_target = alignment[i].split("-");
    nt_label.push(output[source_target[1]]);
  }
  
  var rule = edge[id][LHS]+"\u2192";
  var children = get_children(id);
  var pos = edge[id][FROM];
  for (var i=0; i<children.length; i++) {
    if (pos != edge[id][FROM]) { rule += " "; }
    var child = children[i];
    for(;pos<edge[child][FROM];pos++) { 
      rule += (input[pos].length <= 10) ? input[pos] : input[pos].substr(0,8) + "."; 
      rule += " ";
    }
    rule += nt_label[i];
    rule += (edge[child][FROM] == edge[child][TO]) ? 
      "[" + edge[child][FROM] + "]" :
      "[" + edge[child][FROM] + "-" + edge[child][TO] + "]";
    pos = edge[child][TO]+1;
  }
  for(;pos<=edge[id][TO];pos++) { 
    if (pos != edge[id][FROM]) { rule += " "; }
    rule += (input[pos].length <= 10) ? input[pos] : input[pos].substr(0,8) + "."; 
  }

  return rule;
}

var rule_list;
var edge2rule;
var current_rule_from = -1;
var current_rule_to;
var RULE_HEIGHT;
var RULE_FONT_SIZE;
var best_hyp_score;
var best_derivation_score;
function show_rules( from, to ) {
  unshow_rules();
  var cell = document.getElementById("cellbox-" + from + "-" + to);
  cell.setAttribute("stroke", "#800000");
  cell.setAttribute("stroke-width", "3");
  current_rule_from = from;
  current_rule_to = to;
  
  best_hyp_score = -9e9;
  best_derivation_score = cell_derivation_score[from][to];
  
  var rule_hash = Array();
  var rule_count = Array();
  rule_list = Array();
  edge2rule = Array();
  for (var i=0;i<cell_hyps[from][to].length;i++) {
    var id = cell_hyps[from][to][i];
    var rule = get_rule( id );
    if (rule_hash[rule] === undefined) {
      rule_hash[rule] = rule_list.length;
      rule_count[rule_list.length] = 1;
      rule_list.push(rule);
    }
    else {
      rule_count[rule_hash[rule]]++;
    }
    edge2rule[id] = rule_hash[rule];
    
    if (edge[id][HYP_SCORE] > best_hyp_score) { 
      best_hyp_score = edge[id][HYP_SCORE]; 
    }
  }
  function sortByRuleCount( a, b ) {
    return rule_count[rule_hash[b]] - rule_count[rule_hash[a]];
  }
  rule_list = rule_list.sort(sortByRuleCount);
  
  RULE_HEIGHT = 15;
  RULE_FONT_SIZE = 11;
  // squeeze if too many rules
  if (rule_list.length * RULE_HEIGHT > (CHART_HEIGHT-50)) {
    var factor = (CHART_HEIGHT-50)/rule_list.length/RULE_HEIGHT;
    RULE_HEIGHT = Math.floor( RULE_HEIGHT * factor );
    RULE_FONT_SIZE = Math.ceil( RULE_FONT_SIZE * factor );
  }
  
  draw_rule_options();
  for(var i=-1; i<rule_list.length; i++) {
    draw_rule(from, to, i);
  }
  if (rule_list.length > 0) {
    click_rule( from, to, 0 );
  }
}

function unshow_rules() {
  if (current_rule_from >= 0) {
    var cell = document.getElementById("cellbox-" + current_rule_from + "-" + current_rule_to);
    cell.setAttribute("stroke", "black");
    cell.setAttribute("stroke-width", "1");
  }
  var finished = 0;
  for(var i=-1; !finished; i++) {
    var old = document.getElementById("rule-" + i);
    if (old != null) { chart.removeChild( old ); }
    else { finished = 1; }
  }
  var old = document.getElementById("rule-message");
  if (old != null) { chart.removeChild( old ); }
  old = document.getElementById("rule-cube");
  if (old != null) { chart.removeChild( old ); }
  finished = 0;
  for(var i=1; !finished; i++) {
    var old = document.getElementById("rule-option-" + i);
    if (old != null) { 
      chart.removeChild( old );
      var old = document.getElementById("rule-option-label-" + i);
      chart.removeChild( old );
    }
    else { finished = 1; }
  }
}

function draw_rule( from, to, rule_id ) {
  var rule_label = document.createElementNS(xmlns,"text");
  rule_label.setAttribute("id", "rule-" + rule_id);
  rule_label.setAttribute("x", CHART_WIDTH-120);
  rule_label.setAttribute("y", 10 + RULE_HEIGHT*(rule_id+1));
  rule_label.setAttribute("text-anchor", "middle");
  if (rule_id>-1) {
    rule_label.setAttribute("style", "font-size: "+RULE_FONT_SIZE+"; font-family: Verdana, Arial;");
    rule_label.setAttribute("onclick","click_rule(" + from + "," + to + "," + rule_id + ");");
    var content = document.createTextNode( rule_list[rule_id] );
    rule_label.appendChild( content );
  }
  else {
    rule_label.setAttribute("style", "font-size: "+(RULE_FONT_SIZE-2)+"; font-family: Verdana, Arial; font-weight: bold;");
    var content = document.createTextNode( rule_list.length == 0 ? "NO RULES" : "RULES" );
    rule_label.appendChild( content );
  }
  chart.appendChild( rule_label );
}

function draw_rule_message( message ) {
  var old = document.getElementById("rule-message");
  if (old != null) { chart.removeChild( old ); }

  var rule_message_group = document.createElementNS(xmlns,"svg");
  rule_message_group.setAttribute("id","rule-message");
  rule_message_group.setAttribute("x", 0);
  rule_message_group.setAttribute("y", 250);
  var line = message.split("<br>");
  for(var i=0;i<line.length;i++) {
    var line_label = document.createElementNS(xmlns,"text");
    line_label.setAttribute("id", "rule-message-line" + id);
    line_label.setAttribute("x", 0);
    line_label.setAttribute("y", RULE_HEIGHT*(i+1));
    line_label.setAttribute("style", "font-size: 9; font-family: Verdana, Arial;");
    var content = document.createTextNode( line[i] );
    line_label.appendChild( content );
    rule_message_group.appendChild( line_label );
  }
  chart.appendChild( rule_message_group );
}

var output_list;
var children_list;
var current_edge;
var current_rule_id = -1;
var axis;
var dimension_order;
var RULE_CUBE_HYP_SIZE;
var RULE_CUBE_FONT_SIZE;
function click_rule( from, to, rule_id ) {
  // highlight current rule
  if (current_rule_id>=0) {
    var rule_label = document.getElementById("rule-"+current_rule_id);
    rule_label.setAttribute("style", "font-size: "+RULE_FONT_SIZE+"; font-family: Verdana, Arial;");    
  }
  var rule_label = document.getElementById("rule-"+rule_id);
  rule_label.setAttribute("style", "font-size: "+RULE_FONT_SIZE+"; font-family: Verdana, Arial; font-weight: bold;");
  current_rule_id = rule_id;
  
  // first get all the data
  output_list = Array();
  var output_hash = Array();
  children_list = Array();
  var children_hash = Array();
  current_edge = Array();
  for (var i=0;i<cell_hyps[from][to].length;i++) {
    var id = cell_hyps[from][to][i];
    var rule = get_rule( id );
    if (rule == rule_list[rule_id]) {
      current_edge.push( id );
      // create index for output (target rhs)
      var output = edge[id][OUTPUT]+"|"+edge[id][HEURISTIC_RULE_SCORE];
      if (output_hash[output] === undefined) {
        output_hash[output] = output_list.length;
        output_list.push(output);
      }
      // create index for children
      var children = get_children( id );
      for(var j=0;j<children.length;j++) {
        // init children indices if needed 
        if (j > children_list.length-1) {
          children_hash.push([]);
          children_list.push([]);
        }
        // build index
        var child = ""+children[j];
        if (children_hash[j][child] === undefined) {
          children_hash[j][child] = children_list[j].length;
          children_list[j].push(parseInt(child));
        }
      }
    }
  }
  
  // sort
  function sortBySecond(a,b) {
    asplit = a.split("|");
    bsplit = b.split("|");
    return bsplit[1] - asplit[1];
  }
  output_list = output_list.sort(sortBySecond);

  function sortHypByScore(a,b) {
    return edge[b][HYP_SCORE] - edge[a][HYP_SCORE];
  }
  for(var i=0;i<children.length;i++) {
    children_list[i].sort(sortHypByScore);
  }
  
  // select dimensions of rule cube
  axis = Array();
  axis.push(output_list);
  for(var i=0;i<children_list.length;i++) {
    axis.push(children_list[i]);
  }

  // determine order
  var dimension_size = Array();
  for(var i=0;i<axis.length;i++) {
    dimension_size.push(i+"|"+(axis[i].length - i/10));
  }
  dimension_size.sort(sortBySecond);
  dimension_order = Array();
  for(var i=0;i<dimension_size.length;i++) {
    id_size = dimension_size[i].split("|");
    dimension_order.push(id_size[0]);
  }

  var z_pos = Array();
  for(i=2;i<axis.length;i++) { z_pos.push(0); }
  var z_pos_string = z_pos.join(",");
  draw_rule_cube(z_pos.join(","));
}

var current_z_pos_string = "";
function draw_rule_cube(z_pos_string) {
  current_z_pos_string = z_pos_string;
  var z_pos = Array();
  if (z_pos_string != "") {
    z_pos = z_pos_string.split(",");
  }
  
  // draw rube cube
  var old = document.getElementById("rule-cube");
  if (old != null) { chart.removeChild( old ); }

  // dimensions of the 2x2 view
  var max_length = axis[dimension_order[0]].length;
  //if (axis.length>1 && axis[dimension_order[1]].length > max_length) {
  //  max_length = axis[dimension_order[1]].length;
  //}

  // space for additional dimensions
  var z_dimension_length = 0;
  if (dimension_order.length > 2) {
    z_dimension_length = -2;
    for(var i=2; i<dimension_order.length; i++ ) {
      z_dimension_length += axis[dimension_order[i]].length + 2;
      max_length += axis[dimension_order[i]].length + 2;
    }
  }
  //if (dimension_order.length > 2) {
  //  for(var i=2; i<dimension_order.length; i++ ) {
  //    if (axis[dimension_order[i]].length > max_z_dimension_length) {
  //      max_z_dimension_length = axis[dimension_order[i]].length;
  //    }
  //  }
  //}
  //if (max_z_dimension_length > 10) {
  //  max_z_dimension_length = 10;
  //}
  //var y_length = axis[dimension_order[0]].length;
  //if (max_z_dimension_length > 0) {
  //  y_length += max_z_dimension_length + 2;
  //  if (y_length > max_length) {
  //    max_length = y_length;
  //  }
  //}

  // calculate table cell and font size
  if (max_length+8 <= CHART_HEIGHT/15) {
    RULE_CUBE_HYP_SIZE = 15;
    RULE_CUBE_FONT_SIZE = 11;
  }
  else if (max_length+8 > CHART_HEIGHT/9) {
    RULE_CUBE_HYP_SIZE = 9;
    RULE_CUBE_FONT_SIZE = 7;    
  }
  else {
    RULE_CUBE_HYP_SIZE = CHART_HEIGHT/(max_length+8);
    RULE_CUBE_FONT_SIZE = (RULE_CUBE_HYP_SIZE * 12/15).toFixed(0);
  }
  var Z_HEIGHT = 0;
  if (dimension_order.length > 2) {
    Z_HEIGHT = (z_dimension_length + 2) * RULE_CUBE_HYP_SIZE;
  }

  var rule_cube = document.createElementNS(xmlns,"svg");
  rule_cube.setAttribute("id","rule-cube");
  rule_cube.setAttribute("x", CHART_WIDTH - 30);
  rule_cube.setAttribute("y", 0);
  chart.appendChild( rule_cube );
  
  // draw y axis
  var label = get_rule_axis_name(dimension_order[0]);
  draw_rule_row(-1,label);
  for(var y=0; y<axis[dimension_order[0]].length; y++) {
    var label = get_rule_axis_label(dimension_order[0], y);
    draw_rule_row(y,label);
  }
  if (axis[dimension_order[0]].length > (CHART_HEIGHT-Z_HEIGHT)/9-10) {
    draw_rule_row(Math.ceil(CHART_HEIGHT/9-10),"(more, "+axis[dimension_order[0]].length+" total)");
  }

  // draw x axis
  if (axis.length > 1) {
    var label = get_rule_axis_name(dimension_order[1]);
    draw_rule_column(-1,label);  
    for(var x=0; x<axis[dimension_order[1]].length && x<CHART_HEIGHT/9-10; x++) {
      var label = get_rule_axis_label(dimension_order[1], x);
      draw_rule_column(x,label);
    }
    if (axis[dimension_order[1]].length > CHART_HEIGHT/9-10) {
      draw_rule_column(Math.ceil(CHART_HEIGHT/9-10),"(more, "+axis[dimension_order[1]].length+" total)");
    }
  }  

  // draw hyps
  for(var y=0; y<axis[dimension_order[0]].length && y<(CHART_HEIGHT-Z_HEIGHT)/9-10; y++) {
    if (axis.length == 1) {
      var hyp = find_hyp_by_rule([y],dimension_order);
      draw_rule_hyp(0,y,hyp);
    }
    else {
      for(var x=0; x<axis[dimension_order[1]].length && x<CHART_HEIGHT/9-10; x++) {
        var hyp = find_hyp_by_rule([y,x].concat(z_pos),dimension_order);
        draw_rule_hyp(x,y,hyp);
      }
    }
  }
      
  
  // draw z-axes
  var pos_offset = axis[dimension_order[0]].length+2;
  for(var z=2;z<dimension_order.length;z++) {
    var label = get_rule_axis_name(dimension_order[z]);
    draw_rule_z(z-2,dimension_order.length-2, z_pos, -1, pos_offset, label);
    for(var i=0;i<axis[dimension_order[z]].length && i<z_dimension_length; i++) {
      var label = get_rule_axis_label(dimension_order[z], i);
      draw_rule_z(z-2,dimension_order.length-2, z_pos, i, pos_offset, label);
    }
    pos_offset += axis[dimension_order[z]].length+2;
  }
  
  // report summary statistics
  var message = output_list.length + " output phrases";
  message += "<br>DEBUG: " + axis.length;
  message += "<br>" + dimension_order.length;
  for(var i=0;i<children_list.length;i++) {
    message += "<br>" + children_list[i].length + " hyps for NT" + (i+1);
  }
  //draw_rule_message(message);  
}

function find_hyp_by_rule(position, dimension_order) {
  for(var e=0;e<current_edge.length;e++) {
    var id = current_edge[e];
    var children = get_children( id );
    var match = 1;
    for(var p=0; p<position.length; p++) {
      if (dimension_order[p] == 0) {
        if (output_list[position[p]] != edge[id][OUTPUT]+"|"+edge[id][HEURISTIC_RULE_SCORE]) { 
          match = 0;
        }
      }
      else {
        var nt_number = dimension_order[p]-1;
        if (children_list[nt_number][position[p]] != children[nt_number]) { 
          match = 0; 
        }
      }
    }
    if (match) { return id; }
  }
  return -1;
}

function get_rule_axis_label( dimension, i ) {
  if (dimension == 0) {
    var output_score = output_list[i].split("|");
    var score = 1 * output_score[1];
    return output_score[0]+" "+score.toFixed(1);
  }
  var id = children_list[dimension-1][i];
  return get_display_output( id ) + " " + edge[id][HYP_SCORE].toFixed(1);
}

function get_rule_axis_name( dimension ) {
  if (dimension == 0) {
    return "TARGET";
  }
  return "NT" + dimension;
}


function get_display_output( id ) {
  var output_string = get_full_output( id );
  var output = output_string.split(" ");
  if (output.length <= 2) {
    return output_string;
  }
  else {
    return output[0] + "\u203B" + output[output.length-1];
  }
}

function get_full_output( id ) {
  var output = edge[id][OUTPUT].split(" ");

  var alignment = edge[id][ALIGNMENT].split(" ");
  var children = get_children( id );
  alignment.sort();
  var nonterminal = Array();
  for(var i=0; i<alignment.length; i++) {
    var source_target = alignment[i].split("-");
    nonterminal[source_target[1]] = children[i];
  }
  
  var full_output = "";
  for(var i=0;i<output.length;i++) {
    if (nonterminal[i] === undefined) {
      full_output += " " + output[i];
    }
    else {
      full_output += " " + get_full_output( nonterminal[i] );
    }
  }
  return full_output.substr(1);
}

function draw_rule_row( pos, label ) {
  var rule_label = document.createElementNS(xmlns,"text");
  rule_label.setAttribute("id", "rule-row-" + pos);
  rule_label.setAttribute("y", RULE_CUBE_FONT_SIZE*10 + RULE_CUBE_HYP_SIZE*pos);
  if (pos>=0) {
    rule_label.setAttribute("style", "font-size: "+RULE_CUBE_FONT_SIZE+"; font-family: Verdana, Arial;");
    rule_label.setAttribute("x", RULE_CUBE_FONT_SIZE*10+5);
  }
  else {
    rule_label.setAttribute("style", "font-size: "+(RULE_CUBE_FONT_SIZE-2)+"; font-family: Verdana, Arial; font-weight: bold;");
    rule_label.setAttribute("x", RULE_CUBE_FONT_SIZE*10-30);
  }
  rule_label.setAttribute("text-anchor", "end");
  var content = document.createTextNode( label );
  rule_label.appendChild( content );
  var rule_cube = document.getElementById("rule-cube");
  rule_cube.appendChild( rule_label );
}

function draw_rule_column( pos, label ) {
  var rule_label = document.createElementNS(xmlns,"text");
  rule_label.setAttribute("id", "rule-column-" + pos);
  rule_label.setAttribute("x", RULE_CUBE_FONT_SIZE*10 -3 + RULE_CUBE_HYP_SIZE*(1+pos) );
  rule_label.setAttribute("y", RULE_CUBE_FONT_SIZE*10 -12);
  rule_label.setAttribute("transform", "rotate(60 "+ (RULE_CUBE_FONT_SIZE*10-3+RULE_CUBE_HYP_SIZE*(1+pos)) +" "+(RULE_CUBE_FONT_SIZE*10 - 12)+")")
  if (pos>=0) {
    rule_label.setAttribute("style", "font-size: "+RULE_CUBE_FONT_SIZE+"; font-family: Verdana, Arial;");
  }
  else {
    rule_label.setAttribute("style", "font-size: "+(RULE_CUBE_FONT_SIZE-2)+"; font-family: Verdana, Arial; font-weight: bold;");
  }
  rule_label.setAttribute("text-anchor", "end");
  var content = document.createTextNode( label );
  rule_label.appendChild( content );
  var rule_cube = document.getElementById("rule-cube");
  rule_cube.appendChild( rule_label );
}

function draw_rule_z( z,total_z, z_pos, pos,pos_offset, label ) {
  var rule_label = document.createElementNS(xmlns,"text");
  rule_label.setAttribute("id", "rule-z-" + z + "-" + pos);
  //rule_label.setAttribute("x", RULE_CUBE_FONT_SIZE*10+10 + CHART_HEIGHT*z/(total_z+1) );
  rule_label.setAttribute("x", RULE_CUBE_FONT_SIZE*10+10 );
  rule_label.setAttribute("y", RULE_CUBE_FONT_SIZE*10 + RULE_CUBE_HYP_SIZE*(pos+pos_offset));
  if (pos >= 0) {
    rule_label.setAttribute("style", "font-size: "+RULE_CUBE_FONT_SIZE+"; font-family: Verdana, Arial;"
      +((z_pos[z] == pos)?" font-weight: bold;":""));
    z_pos_copy = z_pos.join(",").split(",");
    z_pos_copy[z] = pos;
    rule_label.setAttribute("onclick","draw_rule_cube(\"" + z_pos_copy.join(",") + "\");");
  }
  else {
    rule_label.setAttribute("style", "font-size: "+(RULE_CUBE_FONT_SIZE-2)+"; font-family: Verdana, Arial; font-weight: bold;");
  }
   
  var content = document.createTextNode( label );
  rule_label.appendChild( content );
  var rule_cube = document.getElementById("rule-cube");
  rule_cube.appendChild( rule_label );
}

function draw_rule_hyp( xpos, ypos, id ) {
  if (id == -1) { return; }
  var diameter = RULE_CUBE_HYP_SIZE-2;
  var hyp = document.createElementNS(xmlns,"circle");
  hyp.setAttribute("id", "rule-hyp-" + id);
  hyp.setAttribute("cx", RULE_CUBE_FONT_SIZE*10+10 + RULE_CUBE_HYP_SIZE*xpos + diameter/2);
  hyp.setAttribute("cy", RULE_CUBE_FONT_SIZE*10-2  + RULE_CUBE_HYP_SIZE*(ypos-0.5) + diameter/2);
  hyp.setAttribute("r", diameter/2);
  hyp.setAttribute("fill", rule_hyp_color(id, 0));
  //hyp.setAttribute("opacity",.5);
  hyp.setAttribute("onmouseover","hover_rule_hyp(" + id + ");")
  hyp.setAttribute("onmouseout","unhover_rule_hyp(" + id + ");")
  var rule_cube = document.getElementById("rule-cube");
  rule_cube.appendChild( hyp );
}

function rule_hyp_color( id, on_off ) {
  if (!show_hyp_score && !show_derivation_score) {
    return hyp_color( id, on_off );
  }
  var inactive_color = on_off ? "80" : "00";
  var hyp_score_color = inactive_color;
  var derivation_score_color = inactive_color;
  if (show_hyp_score) {
    hyp_score_color = get_score_from_color(best_hyp_score-edge[id][HYP_SCORE]);
  }
  if (show_derivation_score) {
    if (edge[id][DERIVATION_SCORE] == null) {
      derivation_score_color = "00";
    }
    else {
      derivation_score_color = get_score_from_color(best_derivation_score-edge[id][DERIVATION_SCORE]);
    }  
  }
  return "#" + inactive_color + derivation_score_color + hyp_score_color;
}

function get_score_from_color( score, on_off ) {
  if (score == null) { return "00"; }
  var dec = 255 - 255 * (score/8);
  if (dec < 0) { dec = 0; }
  if (on_off) { dec = dec/2+128; }
  dec = Math.floor(dec/16)*16+15;
  var color = dec.toString(16);
  if (dec < 16) { color = "0"+color; }
  return color;
}
