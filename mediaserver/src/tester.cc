
#include "common.h"

#include "upnp_xml.h"
#include "storage.h"
#include "cds_objects.h"
#include "process.h"

#include <stdio.h>


using namespace mxml;
using namespace zmm;

void test_parsing(String file)
{

    Ref<Parser> parser(new Parser());
    Ref<Element> root;
    try
    {
        root = parser->parseFile(file);
        log_info(("parsing successfull\n"));
        String xml = root->print();
        log_info(("%s\n", xml.c_str()));
    }
    catch(ParseException pe)
    {
        String msg = String("Parse error: ")+ pe.getMessage() +" at "+ pe.context->location
                + " line "+ pe.context->line;
        flog_info((stderr, "%s\n", msg.c_str()));
    }
}

void test_construction()
{
    Ref<Element> item(new Element("item"));
    
    item->addAttribute("id", "01");
    item->addAttribute("parentID", "0");
    item->addAttribute("restricted", "1");
       
    item->appendTextChild("dc:title", "Addicted To Vaginal Skin");
    item->appendTextChild("upnp:class", "object.item.audioItem.musicTrack");

    Ref<Element> res(new Element("res"));
    
    res->addAttribute("protocolInfo", "http-get:*:audio/mpeg:*");

    item->appendChild(res);
    
    String xml = item->print();

    Ref<Element> test;
    test = UpnpXML_CreateResponse("MyActionName", "MyServiceType");
    String test_str = test->print();

    log_info(("%s\n%s\n", xml.c_str(), test_str.c_str()));
}

void test_storage()
{
    Ref<Storage> storage = Storage::getInstance();

    // Ref<BrowseParam> param(new BrowseParam("0", BROWSE_METADATA));
    Ref<BrowseParam> param(new BrowseParam("100", BROWSE_DIRECT_CHILDREN));
    Ref<Array<CdsObject> > arr = storage->browse(param);
    for(int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        switch(obj->getObjectType())
        {
            case OBJECT_TYPE_CONTAINER:
            {
                Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
                log_info(("container: ID: %s, parent_ID: %s, dc_title: %s update_id: %d\n",
                    cont->getID().c_str(),
                    cont->getParentID().c_str(),
                    cont->getTitle().c_str(),
                    cont->getUpdateID()
                ));
                break;
            }
            case OBJECT_TYPE_ITEM:
            case (OBJECT_TYPE_ITEM | OBJECT_TYPE_ACTIVE_ITEM):
            {
                Ref<CdsItem> item = RefCast(obj, CdsItem);
                log_info(("item: ID: %s, parent_ID: %s, dc_title: %s\n",
                    item->getID().c_str(),
                    item->getParentID().c_str(),
                    item->getTitle().c_str()
                ));
                break;
            }
            default:
            {
                log_info(("unknown file type: %d\n", obj->getObjectType()));
                exit(1);
            }
        }
    }
}

void never_call()
{
    throw StorageException("fuck");
}

void test_process()
{
    String str("[String sent to process]");
    String res = run_process("./trigger", "run", str);

    log_info(("RC: %d\n", res.getBase()->getRefCount()));

    log_info(("Server: returned: %s\n", res.c_str()));
}

int main(int argc, char **argv)
{
    String file = argv[1];
    // test_parsing(file);
    // test_construction();

    // test_storage();
    
    test_process();
}

