/** \file
 *
 * Document metadata dialog, Gtkmm-style
 *
 * Authors:
 *   bulia byak <buliabyak@users.sf.net>
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Jon Phillips <jon@rejon.org>
 *   Ralf Stephan <ralf@ark.in-berlin.de> (Gtkmm)
 *
 * Copyright (C) 2000 - 2006 Authors
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif



#include "ui/widget/entity-entry.h"

#include "xml/node-event-vector.h"
#include "dialogs/rdf.h"

#include "inkscape.h"
#include "verbs.h"
#include "desktop-handles.h"
#include "desktop.h"
#include "sp-namedview.h"

#include "document-metadata.h"

//using std::pair;

namespace Inkscape {
namespace UI {
namespace Dialog {

#define SPACE_SIZE_X 15
#define SPACE_SIZE_Y 15

//===================================================

//---------------------------------------------------

static DocumentMetadata *_instance = 0;

static void on_repr_attr_changed (Inkscape::XML::Node *, gchar const *, gchar const *, gchar const *, bool, gpointer);
static void on_doc_replaced (SPDesktop* dt, SPDocument* doc);
static void on_activate_desktop (Inkscape::Application *, SPDesktop* dt, void*);
static void on_deactivate_desktop (Inkscape::Application *, SPDesktop* dt, void*);

static Inkscape::XML::NodeEventVector const _repr_events = {
    NULL, /* child_added */
    NULL, /* child_removed */
    on_repr_attr_changed,
    NULL, /* content_changed */
    NULL  /* order_changed */
};


DocumentMetadata*
DocumentMetadata::create()
{
    if (_instance) return _instance;
    _instance = new DocumentMetadata;
    _instance->init();
    return _instance;
}

void
DocumentMetadata::destroy()
{
    if (_instance)
    {
        delete _instance;
        _instance = 0;
    }
}

DocumentMetadata::DocumentMetadata() 
    : Dialog ("dialogs.documentmetadata", SP_VERB_DIALOG_METADATA),
      _page_metadata1(1, 1), _page_metadata2(1, 1),
      _prefs_path("dialogs.documentmetadata")
{
    hide();
    set_resizable (false);
    _tt.enable();
    get_vbox()->set_spacing (4);
    get_vbox()->pack_start (_notebook, true, true);

    _notebook.append_page(_page_metadata1, _("Metadata"));
    _notebook.append_page(_page_metadata2, _("License"));

    build_metadata();
}

void
DocumentMetadata::init()
{
    update();

    Inkscape::XML::Node *repr = SP_OBJECT_REPR(SP_DT_NAMEDVIEW(SP_ACTIVE_DESKTOP));
    repr->addListener (&_repr_events, this);

    _doc_replaced_connection = SP_ACTIVE_DESKTOP->connectDocumentReplaced (sigc::ptr_fun (on_doc_replaced));

    g_signal_connect(G_OBJECT(INKSCAPE), "activate_desktop",
                     G_CALLBACK(on_activate_desktop), 0);
    
    g_signal_connect(G_OBJECT(INKSCAPE), "deactivate_desktop",
                     G_CALLBACK(on_deactivate_desktop), 0);
    
    show_all_children();
    present();
}

DocumentMetadata::~DocumentMetadata() 
{
    Inkscape::XML::Node *repr = SP_OBJECT_REPR(SP_DT_NAMEDVIEW(SP_ACTIVE_DESKTOP));
    repr->removeListenerByData (this);
    _doc_replaced_connection.disconnect();

    for (RDElist::iterator it = _rdflist.begin(); it != _rdflist.end(); it++)
        delete (*it);
}

//========================================================================

/**
 * Helper function that attachs widgets in a 3xn table. The widgets come in an
 * array that has two entries per table row. The two entries code for four
 * possible cases: (0,0) means insert space in first column; (0, non-0) means
 * widget in columns 2-3; (non-0, 0) means label in columns 1-3; and
 * (non-0, non-0) means two widgets in columns 2 and 3.
**/
inline void
attach_all (Gtk::Table &table, const Gtk::Widget *arr[], unsigned size, int start = 0)
{
    for (unsigned i=0, r=start; i<size/sizeof(Gtk::Widget*); i+=2)
    {
        if (arr[i] && arr[i+1])
        {
            table.attach (const_cast<Gtk::Widget&>(*arr[i]),   1, 2, r, r+1, 
                      Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
            table.attach (const_cast<Gtk::Widget&>(*arr[i+1]), 2, 3, r, r+1, 
                      Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
        }
        else
        {
            if (arr[i+1])
                table.attach (const_cast<Gtk::Widget&>(*arr[i+1]), 1, 3, r, r+1, 
                      Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
            else if (arr[i])
            {
                Gtk::Label& label = reinterpret_cast<Gtk::Label&> (const_cast<Gtk::Widget&>(*arr[i]));
                label.set_alignment (0.0);
                table.attach (label, 0, 3, r, r+1, 
                      Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
            }
            else
            {
                Gtk::HBox *space = manage (new Gtk::HBox);
                space->set_size_request (SPACE_SIZE_X, SPACE_SIZE_Y);
                table.attach (*space, 0, 1, r, r+1, 
                      (Gtk::AttachOptions)0, (Gtk::AttachOptions)0,0,0);
            }
        }
        ++r;
    }
}

void
DocumentMetadata::build_metadata()
{
    _page_metadata1.show();

    Gtk::Label *label = manage (new Gtk::Label);
    label->set_markup (_("<b>Dublin Core Entities</b>"));
    label->set_alignment (0.0);
    _page_metadata1.table().attach (*label, 0,3,0,1, Gtk::FILL, (Gtk::AttachOptions)0,0,0);
     /* add generic metadata entry areas */
    struct rdf_work_entity_t * entity;
    int row = 1;
    for (entity = rdf_work_entities; entity && entity->name; entity++, row++) {
        if ( entity->editable == RDF_EDIT_GENERIC ) {
            EntityEntry *w = EntityEntry::create (entity, _tt, _wr);
            _rdflist.push_back (w);
            Gtk::HBox *space = manage (new Gtk::HBox);
            space->set_size_request (SPACE_SIZE_X, SPACE_SIZE_Y);
            _page_metadata1.table().attach (*space, 0,1, row, row+1, Gtk::FILL, (Gtk::AttachOptions)0,0,0);
            _page_metadata1.table().attach (w->_label, 1,2, row, row+1, Gtk::FILL, (Gtk::AttachOptions)0,0,0);
            _page_metadata1.table().attach (*w->_packable, 2,3, row, row+1, Gtk::FILL|Gtk::EXPAND, (Gtk::AttachOptions)0,0,0);
        }
    }

    _page_metadata2.show();

    row = 0;
    Gtk::Label *llabel = manage (new Gtk::Label);
    llabel->set_markup (_("<b>License</b>"));
    llabel->set_alignment (0.0);
    _page_metadata2.table().attach (*llabel, 0,3, row, row+1, Gtk::FILL, (Gtk::AttachOptions)0,0,0);
    /* add license selector pull-down and URI */
    ++row;
    _licensor.init (_tt, _wr);
    Gtk::HBox *space = manage (new Gtk::HBox);
    space->set_size_request (SPACE_SIZE_X, SPACE_SIZE_Y);
    _page_metadata2.table().attach (*space, 0,1, row, row+1, Gtk::FILL, (Gtk::AttachOptions)0,0,0);
    _page_metadata2.table().attach (_licensor, 1,3, row, row+1, Gtk::EXPAND|Gtk::FILL, (Gtk::AttachOptions)0,0,0);
}

/**
 * Update dialog widgets from desktop.
 */
void
DocumentMetadata::update()
{
    if (_wr.isUpdating()) return;
    
    _wr.setUpdating (true);
    set_sensitive (true);

    //-----------------------------------------------------------meta pages
    /* update the RDF entities */
    for (RDElist::iterator it = _rdflist.begin(); it != _rdflist.end(); it++)
        (*it)->update (SP_ACTIVE_DOCUMENT);
        
    _licensor.update (SP_ACTIVE_DOCUMENT);

    _wr.setUpdating (false);
}

//--------------------------------------------------------------------

void
DocumentMetadata::on_response (int id)
{
    if (id == Gtk::RESPONSE_CLOSE)
        hide();
}

/**
 * Called when XML node attribute changed; updates dialog widgets.
 */
static void
on_repr_attr_changed (Inkscape::XML::Node *, gchar const *, gchar const *, gchar const *, bool, gpointer)
{
    if (!_instance)
        return;

    _instance->update();
}

static void 
on_activate_desktop (Inkscape::Application *, SPDesktop* dt, void*)
{
    if (!_instance)
        return;

    Inkscape::XML::Node *repr = SP_OBJECT_REPR(SP_DT_NAMEDVIEW(SP_ACTIVE_DESKTOP));
    repr->addListener (&_repr_events, _instance);
    _instance->_doc_replaced_connection = SP_ACTIVE_DESKTOP->connectDocumentReplaced (sigc::ptr_fun (on_doc_replaced));
    _instance->update();
}

static void 
on_deactivate_desktop (Inkscape::Application *, SPDesktop* dt, void*)
{
    if (!_instance)
        return;

    Inkscape::XML::Node *repr = SP_OBJECT_REPR(SP_DT_NAMEDVIEW(SP_ACTIVE_DESKTOP));
    repr->removeListenerByData (_instance);
    _instance->_doc_replaced_connection.disconnect();
}

static void 
on_doc_replaced (SPDesktop* dt, SPDocument* doc)
{
    if (!_instance)
        return;

    Inkscape::XML::Node *repr = SP_OBJECT_REPR(SP_DT_NAMEDVIEW(dt));
    repr->addListener (&_repr_events, _instance);
    _instance->update();
}


} // namespace Dialog
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=c++:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
