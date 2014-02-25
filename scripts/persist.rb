#!/usr/bin/env ruby
require 'socket'
require 'pathname'
require 'thread'
require 'shellwords'
throw "Expected MOSES environment variable" unless ENV["MOSES"]
args={}
cur = nil
ARGV.each do |a|
  if a[0..0] == '-' and (a[1] > '9'[0] || a[1] < '0'[0]) then
    args[a[1..-1]] = []
    cur = args[a[1..-1]]
  else
    cur << a
  end
end

socket = nil
if File.exist?("/tmp/socket")
  socket = UNIXSocket.new("/tmp/socket")
else
  Process.fork do
    serv = UNIXServer.new("/tmp/socket")
    line = ARGV - ["-show-weights"]
    bad = line.index("-input-file")
    if bad
      line.delete_at(bad)
      #and the argument
      line.delete_at(bad)
    end
    bad = line.index("-n-best-list")
    if bad
      line.delete_at(bad)
      line.delete_at(bad)
      line.delete_at(bad)
      line.delete_at(bad) if line[bad] == "distinct"
    end
    
    command = ENV["MOSES"] + ' ' + Shellwords.join(line) + " -n-best-list /dev/null 100"
    $stderr.puts "Launching " + command
    moses = IO.popen(command, "w+")
    while c = serv.accept
      c.send_io(moses)
      while l = c.gets
        moses.puts l
      end
    end
    c.close
  end

  60.times do
    begin
      sleep 1
      socket = UNIXSocket.new("/tmp/socket")
      break
    rescue Errno::ENOENT
    end
  end
  throw "Failed to connect" unless socket
end

reader = socket.recv_io

if args["show-weights"]
  putter = Thread.new do 
    socket.puts "show-weights"
  end
  while l = reader.gets
    break if l.strip == "END WEIGHTS"
    $stdout.puts l
  end
  putter.join
  exit 0
end

queue = Queue.new

t = Thread.new do
  begin
    while queue.pop
      puts reader.gets
    end
    line = reader.gets
    unless line == "Waited\n"
      $stderr.puts "Bad wait " + line
      exit 1
    end
  rescue
    $stderr.puts $!
    exit 2
  end
end

instruct = [
  "/dev/stdout", #1-best
  Dir.pwd + '/' + args["n-best-list"][0], #n-best
  args["weight-file"] ? Pathname.new(args["weight-file"][0]).realpath.to_s : "/dev/null", #sparse weights
  args["weight-overwrite"] #dense weights
].join(' ')

socket.puts "reweight " + instruct
name = args["input-file"][0]
File.new(name).each do |l|
  socket.puts "translate " + l
  queue << 1
end
socket.puts "wait"
socket.close_write
queue << nil
t.join
