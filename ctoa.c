/**
 * synopsis: Parse a gpx file and delete all dissent elevations
 * purpose: Parse a gpx file and delete all dissent elevations
 * usage: just parse as first argument the xml file
 * author: Christopher Loessl
 * copy: GPLv2 or later
 */
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_TREE_ENABLED

/*
 *To compile this file using gcc you can type
 *gcc `xml2-config --cflags --libs` -o ctoa ctoa.c
 */

/**
 * print_element_names:
 * @a_node: the initial xml node to consider.
 *
 * Prints the lat, lon, ele of all the xml elements
 * that are siblings or children of a given xml node.
 */
static void
print_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;
    xmlChar *attlat;
    xmlChar *attlon;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        //printf("node type: %d, name: %s\n", cur_node->type, cur_node->name);
        if (cur_node->type == XML_ELEMENT_NODE) {

            if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"trkpt"))) {
                attlat = xmlGetProp(cur_node, "lat");
                attlon = xmlGetProp(cur_node, "lon");
                printf("Att: lat = %s \t lon = %s \t ele = %s\n", attlat, attlon, cur_node->children->next->children->content);
                //printf("node type: %d, name: %s\n", cur_node->type, cur_node->name);
                //printf("---->>> %s <<<----\n", cur_node->children->next->children->content);
                xmlFree(attlat);
                xmlFree(attlon);
            }
        }/* else {
        ^    printf("Else: Ich bin name: %s und ich bin content: %s <-\n", cur_node->name, cur_node->content);
        //        printf("Type: %d und String: %s und name: %s und parent: %s\n", cur_node->type, cur_node->content, cur_node->name, cur_node->parent->name);
        //    if (xmlStrcmp(cur_node->parent->parent->name, (const xmlChar *)"ele")) {
        //        printf("Type: %d und String: %s \n", cur_node->type, cur_node->content);
        //    }
        }*/

        //printf("\n\ncur_node->children\n\n\n");
        print_element_names(cur_node->children);
    }
}


/**
 * Simple example
 * walk down the DOM, and print lat, lon, ele
 */
int
main(int argc, char **argv)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    if (argc != 2)
        return(1);

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /*parse the file and get the DOM */
    doc = xmlReadFile(argv[1], NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse file %s\n", argv[1]);
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

    print_element_names(root_element);

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
