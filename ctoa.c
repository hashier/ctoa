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

#define MIN -8000

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



static double
getCurrentElevation(xmlNode *trkptNode) {

	xmlNode *tmp_node;

	// Find the elevation child and data of a trkpt
	for (tmp_node = trkptNode->children; tmp_node; tmp_node = tmp_node->next) {
		if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"ele")) && (tmp_node->children) ) {
			return ( atof((const char *)tmp_node->children->content) );
		}
	}
	fprintf(stderr, "Warning: Found trackpoint without elevation data, are you sure this is correct?\n");
	return MIN;
}

static double
getPrevElevation(xmlNode *trkptNode, int offset) {

	xmlNode *tmp_node;
	int counter = 1;

	if ( offset == 0 )
		return ( getCurrentElevation( trkptNode) );

	// Just find next trkpt point and call getCurrentElevation
	for (tmp_node = trkptNode->prev; tmp_node; tmp_node = tmp_node->prev) {
		if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"trkpt")) && (tmp_node->children) ) {
			if ( counter < offset) {
				counter++;
				continue;
			}
			return ( getCurrentElevation( tmp_node) );
		}
	}
	//fprintf(stderr, "Warning: Found trackpoint without elevation data, are you sure this is correct?\n");
	return MIN;
}

static double
getNextElevation(xmlNode *trkptNode, int offset) {

	xmlNode *tmp_node;
	int counter = 1;

	if ( offset == 0 )
		return ( getCurrentElevation( trkptNode) );

	// Just find next trkpt point and call getCurrentElevation
	for (tmp_node = trkptNode->next; tmp_node; tmp_node = tmp_node->next) {
		if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"trkpt")) && (tmp_node->children) ) {
			if ( counter < offset) {
				counter++;
				continue;
			}
			return ( getCurrentElevation( tmp_node) );
		}
	}
	//fprintf(stderr, "Warning: Found trackpoint without elevation data, are you sure this is correct?\n");
	return MIN;
}

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
	int i;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		ele = 0;
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

				ele = getCurrentElevation(cur_node);

				if ( config->printOutLatLonEle == 1 ) {
					printf("Att: lat = %s \t lon = %s \t ele = %.3f\n", attlat, attlon, ele);
					//printf("Att: lat = %s \t lon = %s \t ele = %.3f - %.3f - %.3f\n", attlat, attlon, getPrevElevation(cur_node), ele, getNextElevation(cur_node));
					//printf("Att: lat = %s \t lon = %s \t ele = %.3f - %.3f\n", attlat, attlon, ele, getPrevElevation(cur_node, 3));
				}

				/* delete? */
				int avg = getCurrentElevation(cur_node);
				for ( i = 1; i < config->radius +1; i++) {
					int tmp = getPrevElevation( cur_node, i);
					//printf("xxxx: %d\n", tmp);
					if ( MIN != tmp )
						avg += tmp;
					else {
						avg += ((config->radius +1) - i ) * getPrevElevation( cur_node, i-1);
						break;
					}
				}
				for ( i = 1; i < config->radius +1; i++) {
					int tmp = getNextElevation( cur_node, i);
					//printf("yyyy: %d\n", tmp);
					if ( MIN != tmp )
						avg += tmp;
					else {
						avg += ((config->radius +1) - i ) * getNextElevation( cur_node, i-1);
						break;
					}
				}

				if ( abs( abs(ele) - abs(( avg / (config->radius*2 +1)) )) > config->factor ) {
					printf("Marked for deletion\n");
					free_node = cur_node;
				}
				xmlFree(attlat);
				xmlFree(attlon);
			}
		}

		traverse_tree(cur_node->children, config);
	}

	if ( free_node != NULL) {
		xmlUnlinkNode(free_node);
		xmlFreeNode(free_node);
	}

}


/**
 * Simple example
 * walk down the DOM, and print lat, lon, ele
 */
int
main(int argc, char **argv)
{

	config config = {8, 32, 0};
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
