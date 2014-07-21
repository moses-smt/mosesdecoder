"""Generates HTML page containing the testresults"""
from testsuite_common import Result, processLogLine, getLastTwoLines
from runtests import parse_testconfig
import os
import sys

from datetime import datetime, timedelta

HTML_HEADING = """<html>
<head>
<title>Moses speed testing</title>
<link rel="stylesheet" type="text/css" href="style.css"></head><body>"""
HTML_ENDING = "</table></body></html>\n"

TABLE_HEADING = """<table><tr class="heading">
  <th>Date</th>
  <th>Time</th> 
  <th>Testname</th>
  <th>Revision</th>
  <th>Branch</th> 
  <th>Time</th>
  <th>Prevtime</th>
  <th>Prevrev</th> 
  <th>Change (%)</th>
  <th>Time (Basebranch)</th> 
  <th>Change (%, Basebranch)</th>
  <th>Time (Days -2)</th> 
  <th>Change (%, Days -2)</th>
  <th>Time (Days -3)</th> 
  <th>Change (%, Days -3)</th>
  <th>Time (Days -4)</th> 
  <th>Change (%, Days -4)</th>
  <th>Time (Days -5)</th> 
  <th>Change (%, Days -5)</th>
  <th>Time (Days -6)</th> 
  <th>Change (%, Days -6)</th>
  <th>Time (Days -7)</th> 
  <th>Change (%, Days -7)</th>
  <th>Time (Days -14)</th> 
  <th>Change (%, Days -14)</th>
  <th>Time (Years -1)</th> 
  <th>Change (%, Years -1)</th>
 </tr>"""

def get_prev_days(date, numdays):
    """Gets the date numdays previous days so that we could search for
    that test in the config file"""
    date_obj = datetime.strptime(date, '%d.%m.%Y').date()
    past_date = date_obj - timedelta(days=numdays)
    return past_date.strftime('%d.%m.%Y')

def gather_necessary_lines(logfile, date):
    """Gathers the necessary lines corresponding to past dates
    and parses them if they exist"""
    #Get a dictionary of dates
    dates = {}
    dates[get_prev_days(date, 2)] = ('-2', None)
    dates[get_prev_days(date, 3)] = ('-3', None)
    dates[get_prev_days(date, 4)] = ('-4', None)
    dates[get_prev_days(date, 5)] = ('-5', None)
    dates[get_prev_days(date, 6)] = ('-6', None)
    dates[get_prev_days(date, 7)] = ('-7', None)
    dates[get_prev_days(date, 14)] = ('-14', None)
    dates[get_prev_days(date, 365)] = ('-365', None)

    openfile = open(logfile, 'r')
    for line in openfile:
        if line.split()[0] in dates.keys():
            day = dates[line.split()[0]][0]
            dates[line.split()[0]] = (day, processLogLine(line))
    openfile.close()
    return dates

def append_date_to_table(resline):
    """Appends past dates to the html"""
    cur_html = '<td>' + str(resline.previous) + '</td>'

    if resline.percentage > 0.05: #If we have improvement of more than 5%
        cur_html = cur_html +  '<td class="better">' + str(resline.percentage) + '</td>'
    elif resline.percentage < -0.05: #We have a regression of more than 5%
        cur_html = cur_html +  '<td class="worse">' + str(resline.percentage) + '</td>'
    else:
        cur_html = cur_html +  '<td class="unchanged">' + str(resline.percentage) + '</td>'
    return cur_html

def compare_rev(filename, rev1, rev2, branch1=False, branch2=False):
    """Compare the test results of two lines. We can specify either a
    revision or a branch for comparison. The first rev should be the
    base version and the second revision should be the later version"""

    #In the log file the index of the revision is 2 but the index of
    #the branch is 12. Alternate those depending on whether we are looking
    #for a specific revision or branch.
    firstidx = 2
    secondidx = 2
    if branch1 == True:
        firstidx = 12
    if branch2 == True:
        secondidx = 12

    rev1line = ''
    rev2line = ''
    resfile = open(filename, 'r')
    for line in resfile:
        if rev1 == line.split()[firstidx]:
            rev1line = line
        elif rev2 == line.split()[secondidx]:
            rev2line = line
        if rev1line != '' and rev2line != '':
            break
    resfile.close()
    if rev1line == '':
        raise ValueError('Revision ' + rev1 + " was not found!")
    if rev2line == '':
        raise ValueError('Revision ' + rev2 + " was not found!")

    logLine1 = processLogLine(rev1line)
    logLine2 = processLogLine(rev2line)
    res = Result(logLine1.testname, logLine1.real, logLine2.real,\
        logLine2.revision, logLine2.branch, logLine1.revision, logLine1.branch)

    return res

def produce_html(path, global_config):
    """Produces html file for the report."""
    html = '' #The table HTML
    for filenam in os.listdir(global_config.testlogs):
        #Generate html for the newest two lines
        #Get the lines from the config file
        (ll1, ll2) = getLastTwoLines(filenam, global_config.testlogs)
        logLine1 = processLogLine(ll1)
        logLine2 = processLogLine(ll2) #This is the life from the latest revision

        #Generate html
        res1 = Result(logLine1.testname, logLine1.real, logLine2.real,\
            logLine2.revision, logLine2.branch, logLine1.revision, logLine1.branch)
        html = html + '<tr><td>' + logLine2.date + '</td><td>' + logLine2.time + '</td><td>' +\
        res1.testname + '</td><td>' + res1.revision[:10] + '</td><td>' + res1.branch + '</td><td>' +\
        str(res1.current) + '</td><td>' + str(res1.previous) + '</td><td>' + res1.prevrev[:10] + '</td>'

        #Add fancy colours depending on the change
        if res1.percentage > 0.05: #If we have improvement of more than 5%
            html = html +  '<td class="better">' + str(res1.percentage) + '</td>'
        elif res1.percentage < -0.05: #We have a regression of more than 5%
            html = html +  '<td class="worse">' + str(res1.percentage) + '</td>'
        else:
            html = html +  '<td class="unchanged">' + str(res1.percentage) + '</td>'

        #Get comparison against the base version
        filenam = global_config.testlogs + '/' + filenam #Get proper directory
        res2 = compare_rev(filenam, global_config.basebranch, res1.revision, branch1=True)
        html = html + '<td>' + str(res2.previous) + '</td>'

        #Add fancy colours depending on the change
        if res2.percentage > 0.05: #If we have improvement of more than 5%
            html = html +  '<td class="better">' + str(res2.percentage) + '</td>'
        elif res2.percentage < -0.05: #We have a regression of more than 5%
            html = html +  '<td class="worse">' + str(res2.percentage) + '</td>'
        else:
            html = html +  '<td class="unchanged">' + str(res2.percentage) + '</td>'

        #Add extra dates comparison dating from the beginning of time if they exist
        past_dates = list(range(2, 8))
        past_dates.append(14)
        past_dates.append(365) # Get the 1 year ago day
        linesdict = gather_necessary_lines(filenam, logLine2.date)

        for days in past_dates:
            act_date = get_prev_days(logLine2.date, days)
            if linesdict[act_date][1] is not None:
                logline_date = linesdict[act_date][1]
                restemp = Result(logline_date.testname, logline_date.real, logLine2.real,\
                logLine2.revision, logLine2.branch, logline_date.revision, logline_date.branch)
                html = html + append_date_to_table(restemp)
            else:
                html = html + '<td>N/A</td><td>N/A</td>'



        html = html + '</tr>' #End row

    #Write out the file
    basebranch_info = '<text><b>Basebranch:</b> ' + res2.prevbranch + ' <b>Revision:</b> ' +\
    res2.prevrev + '</text>'
    writeoutstr = HTML_HEADING + basebranch_info + TABLE_HEADING + html + HTML_ENDING
    writefile = open(path, 'w')
    writefile.write(writeoutstr)
    writefile.close()

if __name__ == '__main__':
    CONFIG = parse_testconfig(sys.argv[1])
    produce_html('index.html', CONFIG)
