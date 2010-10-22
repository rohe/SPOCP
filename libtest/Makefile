SUBDIRS = \
	test_oct \
	test_parr \
	test_skiplist \
	test_octarr \
	test_ll \
	test_verify \
	test_sexp \
    test_parse \
	test_hashfunc \
	test_basic \
	test_boundary \
	test_boundarysl \
	test_range \
	test_varr \
	test_dset \
	test_strfunc \
    test_sexptool \
	test_cachetime \
	test_atom \
	test_bsdrb \
	test_becpool \
	test_cache \
	test_plugin \
	test_bcexp \
	test_ruleinst \
	test_ruleinfo \
	test_check \
	test_dback \
	test_element \
    test_element_I \
	test_rvapi \
	test_subelem \
	test_bcond \
    test_rangestore \
    test_index \
    test_resset \
	test_branch \
	test_junc \
	test_hash \
    test_ssn \
	test_ssn_b \
    test_db0 \
	test_match \
    test_aci \
    test_atommatch \
    test_list \
	test_rm \
    test_dbapi \
    test_spocp
	
#   test_complex_free
#   test_eval

# -- don't ---
#   test_display
#   test_element_display
#   test_rdb
#   test_rdwr
#   test_io
#   test_spocp_result
# -- don't ---


all: all-recursive 

all-recursive:
	for dir in $(SUBDIRS); \
		do cd $$dir; \
		echo "========================= $$dir ========================="; \
		$(MAKE) run; \
		cd ..; \
	done

clean: clean-recursive

clean-recursive:
	for dir in $(SUBDIRS); \
		do cd $$dir; \
		$(MAKE) clean; \
		cd ..; \
	done
