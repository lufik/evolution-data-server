# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000-2002
#	Sleepycat Software.  All rights reserved.
#
# $Id: test079.tcl,v 1.1.1.1 2003/11/20 22:14:02 toshok Exp $
#
# TEST	test079
# TEST	Test of deletes in large trees.  (test006 w/ sm. pagesize).
# TEST
# TEST	Check that delete operations work in large btrees.  10000 entries
# TEST	and a pagesize of 512 push this out to a four-level btree, with a
# TEST	small fraction of the entries going on overflow pages.
proc test079 { method {nentries 10000} {pagesize 512} {tnum 79} args} {
	if { [ is_queueext $method ] == 1 } {
		set method  "queue";
		lappend args "-extent" "20"
	}
	eval {test006 $method $nentries 1 $tnum -pagesize $pagesize} $args
}
