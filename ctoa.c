/**
 * synopsis: Parse a gpx file and delete all dissent elevations
 * purpose: Parse a gpx file and delete all dissent elevations
 * usage: just parse as first argument the xml file or use -h for more help
 * author: Christopher Loessl
 * mail: cloessl@x-berg.de
 * copyright: GPLv2 or later
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_TREE_ENABLED

#define DEBUG

#ifndef DEBUG
  #define printDebugMsg(msg) ((void)0)
  #define printDebugMsgv(msg) ((void)0)
#else
  #define printDebugMsg(msg) (printDebug(msg))
  #define printDebugMsgv(msg) (printDebugv(msg, __FILE__, __LINE__))
#endif

void printDebugv( const char *msg, const char *file, const int line) {
	fprintf(stderr, "%s : line %d in %s\n", msg, line, file);
}
void printDebug( const char *msg) {
	fprintf(stderr, "%s\n", msg);
}

typedef struct _config {
	int radius;
	int factor;
	int printOutLatLonEle;
} config;

/*
 *To compile this file using gcc you can type
 *gcc `xml2-config --cflags --libs` -o ctoa ctoa.c
 */

/**
 * traverse_tree:
 * @a_node: the initial xml node to consider.
 * @config: config
 *
 * Prints the lat, lon, ele of all the xml elements
 * that are siblings or children of a given xml node.
 */
static void
traverse_tree(xmlNode * a_node, const config *config) {

	xmlNode *cur_node = NULL;
	xmlNode *free_node = NULL;
	xmlChar *attlat;
	xmlChar *attlon;
	double ele = 0;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		/*
		 * XML_ELEMENT_NODE = 1
		 * XML_ATTRIBUTE_NODE = 2
		 * XML_TEXT_NODE = 3
		*/
		//printf("Type: %d  String: %s  name: %s  parent: %s\n", cur_node->type, cur_node->content, cur_node->name, cur_node->parent->name);
		if (cur_node->type == XML_ELEMENT_NODE) {

			if ( (!xmlStrcmp(cur_node->name, (const xmlChar *)"trkpt")) ) {
				attlat = xmlGetProp(cur_node, (const xmlChar *)"lat");
				attlon = xmlGetProp(cur_node, (const xmlChar *)"lon");
				ele = atof((const char *)cur_node->children->next->children->content);
				if ( config->printOutLatLonEle == 1 ) {
					printf("Att: lat = %s \t lon = %s \t ele = %.3f\n", attlat, attlon, ele);
				}
				if ( ele == 89.845 ) {
					free_node = cur_node;
				}
				xmlFree(attlat);
				xmlFree(attlon);
			}
		} else {
		}

		traverse_tree(cur_node->children, config);
	}
	xmlUnlinkNode(free_node);
	xmlFreeNode(free_node);
}


/**
 * Simple example
 * walk down the DOM, and print lat, lon, ele
 */
int
main(int argc, char **argv)
{

	config config = {2, 3, 0};
	extern char *optarg;
	extern int optind, opterr, optopt;
	int opt;
	char *infile = NULL, *outfile = NULL;

	xmlNode *root_element;
	xmlDoc *doc = NULL;

	/*
	 * this initialize the library and check potential ABI mismatches
	 * between the version it was compiled for and the actual shared
	 * library used.
	 */
	LIBXML_TEST_VERSION

	while ((opt = getopt(argc, argv, "hi:o:r:f:v")) != -1) {
		switch (opt) {
			case 'h': // help
				fprintf(stderr, "Usage: %s -i <infile> -o <outfile> [-v] [-r radius] [-f factor]\n", argv[0]);
				exit (EXIT_FAILURE);
				break;
			case 'i': // infile
				infile = optarg;
				break;
			case 'o': // outfile
				outfile = optarg;
				break;
			case 'r': // radius
				config.radius = atoi(optarg);
				break;
			case 'f': // factor
				config.factor = atoi(optarg);
				break;
			case 'v':
				config.printOutLatLonEle = 1;
				break;
			default: // ?
				fprintf(stderr, "Usage: %s -i <infile> -o <outfile> [-v] [-r radius] [-f factor]\n", argv[0]);
				exit (EXIT_FAILURE);
				break;
		}
	}

	/*parse the file and get the DOM */
	doc = xmlReadFile(infile, NULL, 0);

	if (doc == NULL) {
		printf("error: could not parse file: \"%s\"\n", infile);
		exit (EXIT_FAILURE);
	}

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);

	traverse_tree(root_element, &config);

	/* Save file */
	if ( outfile != NULL ) {
		xmlSaveFile(outfile, doc);
	}

	/*free the document */
	xmlFreeDoc(doc);

	/*
	 *Free the global variables that may
	 *have been allocated by the parser.
	 */
	xmlCleanupParser();

	return 0;
}
#else
int main(void) {
	fprintf(stderr, "Tree support not compiled in\n");
	exit(1);
}
#endif
