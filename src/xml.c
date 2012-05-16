#define DEBUG_XML

#include "vmrunner.h"

#ifdef DEBUG_XML
#define DPRINTF(fmt, ...) \
do { char *dtmp = get_datetime(); fprintf(stderr, "[%s ", dtmp); free(dtmp); dtmp=NULL; fprintf(stderr, "pvr/xml        ] " fmt , ## __VA_ARGS__); fflush(stderr); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

char *xml_query_by_id(char *xmlFile, char *xPath, int id) {
	xmlDocPtr doc;
	xmlParserCtxtPtr pctxt;
	xmlXPathContextPtr context;
	xmlXPathObjectPtr op;
	xmlNodeSetPtr nodeset;
	char *data = NULL;
	int num;

	if ((xmlFile == NULL) || (strlen(xmlFile) < 1))
		return NULL;

	if (xmlFile[0] != '<') {
		if (access(xmlFile, R_OK) != 0) {
			fprintf(stderr, "Error: File %s doesn't exist or is not accessible for reading.\n", xmlFile);
			return NULL;
		}

		doc = xmlParseFile(xmlFile);
	}
	else {
		pctxt = xmlCreateDocParserCtxt((xmlChar *)xmlFile);
		doc = xmlCtxtReadDoc(pctxt, (xmlChar *)xmlFile, NULL, NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR);

		/* A little hack, but working */
		do { char tmp[1024] = { 0 }; char *dtmp = get_datetime(); snprintf(tmp, sizeof(tmp), "[%s ", dtmp); free(dtmp); dtmp=NULL; } while (0);
	}

	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		DPRINTF("Error in xmlXPathNewContext\n");
		return NULL;
	}

	DPRINTF("Trying to access xPath node %s (pos %d)\n", xPath, id);
	op = xmlXPathEvalExpression( (xmlChar *)xPath, context);
	xmlXPathFreeContext(context);
	if (op == NULL) {
		DPRINTF("Error in xmlXPathEvalExpression\n");
		return NULL;
	}

	if(xmlXPathNodeSetIsEmpty(op->nodesetval)){
		xmlXPathFreeObject(op);
		DPRINTF("No result\n");
		return NULL;
	}

	nodeset = op->nodesetval;
	num = nodeset->nodeNr;

#if 0
	for (i = 0; i < num; i++) {
		data = (char *)xmlNodeListGetString(doc, (nodeset->nodeTab[i])->xmlChildrenNode, 1);
		DPRINTF("%d. >>> %s\n", i, data);
	}
#endif

	DPRINTF("Current num value is %d, id value is %d\n", num, id);
	if (num > id) {
		char *tmp = (char *)xmlNodeListGetString(doc, (nodeset->nodeTab[id])->xmlChildrenNode, 1);

		data = strdup(tmp);
		DPRINTF("Got data element of '%s'\n", tmp);
	}
	else
		DPRINTF("Trying to access element out of bounds (id > num)\n");

	xmlXPathFreeObject(op);

	xmlFreeDoc(doc);
	xmlCleanupParser();

	return data;
}

char *xml_query(char *xmlFile, char *xPath) {
	return xml_query_by_id(xmlFile, xPath, 0);
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
