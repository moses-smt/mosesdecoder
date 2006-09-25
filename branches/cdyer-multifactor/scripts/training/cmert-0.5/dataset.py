#!/usr/bin/python2.3

'''Decoder interface:

Dataset.process() expects a function, which in turn takes a Sentence as input
and produces a Sentence or list of Sentences as output.

The input Sentence will be marked with the <seg> tag it was found in
the input file with.

The output Sentences should be marked with <seg> tags if they are to
be marked as such in the output file.
'''

import sys, sgmllib, xml.sax.saxutils, log

def attrs_to_str(d):
    if len(d) == 0:
        return ""
    l = [""]+["%s=%s" % (name, xml.sax.saxutils.quoteattr(value)) for (name, value) in d]
    return " ".join(l)

def attrs_to_dict(a):
    d = {}
    for (name, value) in a:
	if d.has_key(name.lower()):
	    raise ValueError, "duplicate attribute names"
	d[name.lower()] = value
    return d

def strip_newlines(s):
    return " ".join(s.split())

class Sentence(object):
    def __init__(self, words=None, meta=None):
        if words is not None:
            self.words = list(words)
        else:
            self.words = []
        if meta is not None:
            self.meta = meta
        else:
            self.meta = []

    def mark(self, tag, attrs):
        self.meta.append((tag, attrs, 0, len(self.words)))

    def getmark(self):
        if len(self.meta) > 0:
            (tag, attrs, i, j) = self.meta[-1]
            if i == 0 and j == len(self.words):
                return (tag, attrs)
            else:
                return None
        else:
            return None

    def unmark(self):
        mark = self.getmark()
        if mark is not None:
            self.meta = self.meta[:-1]
        return mark

    def __cmp__(self, other):
        return cmp((self.words, self.meta), (other.words, other.meta))

    def __str__(self):
        def cmp_spans((tag1,attr1,i1,j1),(tag2,attr2,i2,j2)):
            if i1==i2<=j1==j2:
                return 0
            elif i2<=i1<=j1<=j2:
                return -1
            elif i1<=i2<=j2<=j1:
                return 1
            else:
                return cmp((i1,j1),(i2,j2)) # don't care
        # this guarantees that equal spans will come out nested
        # we want the later spans to be outer
        # this relies on stable sort
        open = [[] for i in xrange(len(self.words)+1)]
        # there seems to be a bug still with empty spans
        empty = [[] for i in xrange(len(self.words)+1)]
        close = [[] for j in xrange(len(self.words)+1)]
        for (tag,attrs,i,j) in sorted(self.meta, cmp=cmp_spans):
            if i == j:
                # do we want these to nest?
                empty[i].append("<%s%s/>" % (tag, attrs_to_str(attrs)))
            open[i].append("<%s%s>" % (tag, attrs_to_str(attrs)))
            close[j].append("</%s>" % tag)

        result = []
        if len(empty[0]) > 0:
            result.extend(empty[0])
        for i in xrange(len(self.words)):
            if i > 0:
                result.append(" ")
            result.extend(reversed(open[i]))
            result.append(self.words[i])
            result.extend(close[i+1])
            if len(empty[i+1]) > 0:
                result.extend(empty[i+1])

        return "".join(result)

    def __add__(self, other):
        if type(other) in (list, tuple):
            return Sentence(self.words + list(other), self.meta)
        else:
            othermeta = [(tag, attrs, i+len(self.words), j+len(self.words)) for (tag, attrs, i, j) in other.meta]
            return Sentence(self.words + other.words, self.meta+othermeta)

def read_raw(f):
    """Read a raw file into a list of Sentences."""
    if type(f) is str:
        f = file(f, "r")
    inputs = []
    i = 0
    for line in f:
        sent = process_sgml_line(line, i)
        sent.mark('seg', [('id',str(i))])
        inputs.append(sent)
        i += 1
    return inputs

class Dataset(object):
    def __init__(self, id=None):
	self.id = id
	self.docs = {}
	self.sysids = []
	self.langs = {}

    def read(self, f):
        '''Read a file into the dataset. Returns (root, sysids)'''
        if type(f) is str:
            f = file(f, "r")
        p = DatasetParser(self)
        p.feed(f.read())
        p.close()
        return (p.root,p.sysids)

    def read_raw(self, f, docid, setid=None, sysid=None, lang=None):
        """Read a raw file into the dataset."""
        if setid is not None:
            if self.id is not None and self.id != setid:
                raise ValueError, "Set ID does not match"
            else:
                self.id = setid
        if sysid not in self.sysids:
            self.sysids.append(sysid)
            self.langs[sysid] = lang
        if type(f) is str:
            f = file(f, "r")
        doc = self.docs.setdefault(docid, Document(docid))
        i = 0
        for line in f:
            if len(doc.segs)-1 < i:
                doc.segs.append(Segment(i))
            if doc.segs[i].versions.has_key(sysid):
                raise ValueError, "multiple versions from same system"
            doc.segs[i].versions[sysid] = process_sgml_line(line, i)
            doc.segs[i].versions[sysid].mark('seg', [('id',str(i))])
            i += 1
        return (None, [sysid])

    def write(self, f, tag, sysids=None):
        if type(f) is str:
            f = file(f, "w")
        f.write(self.string(tag, sysids))

    def write_raw(self, f, sysid=None):
        if type(f) is str:
            f = file(f, "w")
        for seg in self.segs():
            f.write(" ".join(seg.versions[sysid].words))
            f.write("\n")

    def string(self, tag, sysids=None):
	if sysids is None:
	    sysids = self.sysids
	elif type(sysids) is str:
	    sysids = [sysids]
	attrs = [('setid', self.id)]
	if self.langs.has_key(None):
	    attrs.append(('srclang', self.langs[None]))
	trglangs = [self.langs[sysid] for sysid in sysids if sysid is not None]
	for lang in trglangs[1:]:
	    if lang != trglangs[0]:
		raise ValueError, "Inconsistent target language"
	if len(trglangs) >= 1:
	    attrs.append(('trglang', trglangs[0]))

        return "<%s%s>\n%s</%s>\n" % (tag, 
				      attrs_to_str(attrs),
				      "".join([doc.string(sysid) for doc in self.docs.values() for sysid in sysids]),
                                      tag)
    
    def process(self, processor, sysid, lang, srcsysid=None):
	if sysid in self.sysids:
	    raise ValueError, "sysid already in use"
	else:
	    self.sysids.append(sysid)
	    self.langs[sysid] = lang
	for seg in self.segs():
            if log.level >= 2:
                sys.stderr.write("Input: %s\n" % str(seg.versions[srcsysid]))
            seg.versions[sysid] = processor(seg.versions[srcsysid])
            if log.level >= 2:
                if type(seg.versions[sysid]) is not list:
                    sys.stderr.write("Output: %s\n" % str(seg.versions[sysid]))
                else:
                    sys.stderr.write("Output (1st): %s\n" % str(seg.versions[sysid][0]))
                        
    def segs(self):
        for doc in self.docs.values():
            for seg in doc.segs:
                yield seg

class Document(object):
    def __init__(self, id):
	self.id = id
	self.segs = []

    def string(self, sysid):
	attrs = [('docid', self.id)]
	if sysid is not None:
	    attrs.append(('sysid', sysid))
	return "<doc%s>\n%s</doc>\n" % (attrs_to_str(attrs),
					"".join([seg.string(sysid) for seg in self.segs]))
	
class Segment(object):
    def __init__(self, id=None):
	self.id = id
	self.versions = {}

    def string(self, sysid):
        v = self.versions[sysid]
        if type(v) is not list:
            v = [v]
        output = []
        for i in xrange(len(v)):
            output.append(str(v[i]))
            output.append('\n')
        return "".join(output)

def process_sgml_line(line, id=None):
    p = DatasetParser(None)
    p.pos = 0
    p.words = []
    p.meta = []
    p.feed(line)
    p.close()
    sent = Sentence(p.words, p.meta)
    return sent

class DatasetParser(sgmllib.SGMLParser):
    def __init__(self, set):
        sgmllib.SGMLParser.__init__(self)
	self.words = None
	self.sysids = []
	self.set = set
        self.mystack = []

    def handle_starttag(self, tag, method, attrs):
        thing = method(attrs)
        self.mystack.append(thing)

    def handle_endtag(self, tag, method):
        thing = self.mystack.pop()
        method(thing)

    def unknown_starttag(self, tag, attrs):
        thing = self.start(tag, attrs)
        self.mystack.append(thing)

    def unknown_endtag(self, tag):
        thing = self.mystack.pop()
        self.end(tag, thing)
        
    def start_srcset(self, attrs):
	attrs = attrs_to_dict(attrs)
	if self.set.id is None:
	    self.set.id = attrs['setid']
	if 0 and self.set.id != attrs['setid']:
	    raise ValueError, "Set ID does not match"
	self.lang = attrs['srclang']
	self.root = 'srcset'
        return None

    def start_refset(self, attrs):
	attrs = attrs_to_dict(attrs)
	if self.set.id is None:
	    self.set.id = attrs['setid']
	if 0 and self.set.id != attrs['setid']:
	    raise ValueError, "Set ID does not match"
	if self.set.langs.setdefault(None, attrs['srclang']) != attrs['srclang']:
	    raise ValueError, "Source language does not match"
	self.lang = attrs['trglang']
	self.root = 'refset'
        return None

    def start_tstset(self, attrs):
	attrs = attrs_to_dict(attrs)
	if self.set.id is None:
	    self.set.id = attrs['setid']
	if 0 and self.set.id != attrs['setid']:
	    raise ValueError, "Set ID does not match"
	if 0 and self.set.langs.setdefault(None, attrs['srclang']) != attrs['srclang']:
	    raise ValueError, "Source language does not match"
	self.lang = attrs['trglang']
	self.root = 'tstset'
        return None

    def end_srcset(self, thing):
        for sysid in self.sysids:
            if sysid not in self.set.sysids:
                self.set.sysids.append(sysid)
                self.set.langs[sysid] = self.lang
    end_refset = end_tstset = end_srcset

    def start_doc(self, attrs):
	attrs = attrs_to_dict(attrs)
	self.doc = self.set.docs.setdefault(attrs['docid'], Document(attrs['docid']))
	self.seg_i = 0
	if self.root == 'srcset':
	    self.sysid = None
	else:
	    self.sysid = attrs['sysid']
        if self.sysid not in self.sysids:
            self.sysids.append(self.sysid)
        return None

    def end_doc(self, thing):
	pass

    def start_seg(self, attrs):
        thing = ('seg', attrs, 0, None)
	attrs = attrs_to_dict(attrs)
	if len(self.doc.segs)-1 < self.seg_i:
	    self.doc.segs.append(Segment(attrs.get('id', None)))
	self.seg = self.doc.segs[self.seg_i]
        if 0 and self.seg.id is not None and attrs.has_key('id') and self.seg.id != attrs['id']:
	    raise ValueError, "segment ids do not match (%s != %s)" % (str(self.seg.id), str(attrs.get('id', None)))
	if self.seg.versions.has_key(self.sysid):
	    raise ValueError, "multiple versions from same system"
	self.pos = 0
        self.words = []
        self.meta = []
        return thing

    def end_seg(self, thing):
        (tag, attrs, i, j) = thing
        self.meta.append((tag, attrs, i, self.pos))
	self.seg_i += 1
	self.seg.versions[self.sysid] = Sentence(self.words, self.meta)
	self.words = None

    """# Special case for start and end of sentence
    def start_s(self, attrs):
        if self.words is not None:
            self.pos += 1
            self.words.append('<s>')
        return None

    def end_s(self, thing):
        if self.words is not None:
            self.pos += 1
            self.words.append('</s>')"""

    def start(self, tag, attrs):
        if self.words is not None:
            return (tag, attrs, self.pos, None)
        else:
            return None

    def end(self, tag, thing):
        if self.words is not None:
            (tag, attrs, i, j) = thing
            self.meta.append((tag, attrs, i, self.pos))

    def handle_data(self, s):
        if self.words is not None:
            words = s.split()
            self.pos += len(words)
	    self.words.extend(words)

if __name__ == "__main__":
    s = Dataset()

    for filename in sys.argv[1:]:
        s.read_raw(filename, 'whatever', 'whatever', filename, 'English')
    s.write(sys.stdout, 'tstset')
