require 'fileutils'
require 'rake/clean'
require 'rubygems'
require 'json'
require 'rainbow/ext/string'

def name(x, post = :o)
  "#{x}/#{x}.#{post}"
end

def getcls(x)
  x.sub(%r{(\w+)/.+}, '\1')
end

def getinc(cls)
  (DEPENDS[name(cls)] or return("-I #{cls}"))[1..-1]
  .map { |x| "-I #{getcls(x)}" }.join(' ')
end

def sys(str, info)
  puts VERBOSE == :detail ? str : info
  system(str)
end

DATALIST = %w(anaspec anaspec_pppc load_dat)

FLIST =  FileList['*'].select { |x| File.exist?(name(x, :h)) } - EXCLUDE
CLIST = FileList['*'].select { |x| File.exist?(name(x, :cc)) } - EXCLUDE
HLIST = FLIST - CLIST

WLIST = FileList[%w(chi2prop mcparas propagator create_source)]
LLIST = CLIST - WLIST

WINC, LINC = FLIST, LLIST + HLIST
INC = WINC.map { |x| "-I #{x}" }

CLEAN.include(CLIST.map { |x| name(x) })
CLEAN.include(FLIST.map { |x| name(x, 'h.gch') })

OBJS = { work: WLIST.map { |x| name(x) }, lin: LLIST.map { |x| name(x) } }

TASKLIST = FileList[:lin, :work]
target = ->(t) { STATIC ? "lib#{t}s.a" : "lib#{t}.so" }

CLOBBER.include(TASKLIST.map { |x| target.call(x) })

task default: TASKLIST

COLLEC = STATIC ? 'ar crs' : "#{CC} #{CFLAG} -shared -fPIC -o"

DEPNAME = 'DEPENDENCY'
def gendepend(srccls)
  puts "Generating depends for #{srccls}"
  res = `#{CC} #{CFLAG} #{INC} -E -MM -fPIC #{name(srccls, :cc)}`.split(' ')
    .select { |x| x != '\\' }#.map { |x| x.sub(/[.]h$/, '.h.gch') } # for precompiling
  [res[0].sub(/^(\w+).o:$/, '\1/\1.o'), res[1..-1]]
end

def gendeps
  file = File.new(DEPNAME, 'w')
  file.puts JSON.generate(Hash[CLIST.map { |x| gendepend(x) }])
  file.close
end

def readdepend
  gendeps unless File.exist?(DEPNAME) && File.size(DEPNAME) != 0
  JSON.parse(File.new(DEPNAME).read)
end

DEPENDS = readdepend

TASKLIST.each do |grp|
  desc "Generating lib#{grp}"
  multitask "#{grp}" => OBJS[grp] do
    targ = target.call(grp)
    sys("#{COLLEC} #{targ} #{OBJS[grp].to_s}", "Link to #{targ}".bright)
  end
end

desc 'Generating (refresh) the dependency for all the classes'
task :dep do
  gendeps
end

desc 'Installing the libraries'
task :install do
  incfile = WINC.map { |x| "#{x}/#{x}.h" }.to_s

  ins = [[incfile, "#{PREFIX}/include"]] + \
    [[Dir.glob('*/enumdef/*def').join(' '), "#{PREFIX}/include/enumdef"]] + \
    CLOBBER.map { |x| File.exist?(x) && [x, "#{PREFIX}/lib"] } + \
    DATALIST.map { |c| ["#{c}/#{c}_data/*", "#{PREFIX}/lib/#{c}_data"] }

  ins.select { |x| x }.each do |o, t|
    FileUtils.mkdir_p(t)
    sys("install -D #{o} #{t}", "Installing:".bright + " #{o}")
  end
end

def compile(cls, t, create = nil)
  dbg = DLIST.include?(cls) ? DEBUG : nil
  datdir = %Q(-D DATDIR=\\"#{PREFIX}/lib/#{cls}_data\\")
  dat = DATALIST.include?(cls) ? datdir : ''

  cmd = "#{CC} #{CFLAG} #{dbg} #{dat} #{getinc(cls)} " + \
    "#{create} -fPIC #{t.source} -o #{t.name}"

  sys(cmd, "Compiling #{t.name}")
end

CLIST.each do |cls|
  file name(cls) => DEPENDS[name(cls)] do |t|
    compile(cls, t, '-c')
  end
end

# For precompiling, it is banned as seems useless
#FLIST.each do |cls|
#  file name(cls, 'h.gch') => name(cls, :h) do |t|
#    compile(cls, t)
#  end
#end


