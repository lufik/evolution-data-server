INCLUDES = 				\
	-I$(top_srcdir)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)/addressbook	\
	-I$(top_builddir)/addressbook	\
	$(EVOLUTION_ADDRESSBOOK_CFLAGS)

#TEST_LIBS=							\
#	$(top_builddir)/addressbook/libebook/libebook-$(API_VERSION).la	\
#	$(EVOLUTION_ADDRESSBOOK_LIBS)
TEST_LIBS=							\
	$(top_builddir)/addressbook/libebook-dbus/libebook.so	\
	$(EVOLUTION_ADDRESSBOOK_LIBS)

noinst_PROGRAMS= test-changes test-categories test-date test-ebook test-ebook-async test-ebook-view test-nonexistent-id test-photo test-query test-self test-string test-undefinedfield test-untyped-phones test-search test-stress-bookviews

test_search_LDADD=$(TEST_LIBS)
test_date_LDADD=$(TEST_LIBS)
test_ebook_LDADD=$(TEST_LIBS)
test_ebook_async_LDADD=$(TEST_LIBS)
test_ebook_view_LDADD=$(TEST_LIBS)
test_changes_LDADD=$(TEST_LIBS)
test_categories_LDADD=$(TEST_LIBS)
test_nonexistent_id_LDADD=$(TEST_LIBS)
test_photo_LDADD=$(TEST_LIBS)
test_query_LDADD=$(TEST_LIBS)
test_self_LDADD=$(TEST_LIBS)
test_string_LDADD=$(TEST_LIBS)
test_undefinedfield_LDADD=$(TEST_LIBS)
test_untyped_phones_LDADD=$(TEST_LIBS)
test_stress_bookviews_LDADD=$(TEST_LIBS)
