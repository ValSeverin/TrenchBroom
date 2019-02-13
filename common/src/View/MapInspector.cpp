/*
 Copyright (C) 2010-2017 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MapInspector.h"

#include "View/BorderLine.h"
#include "View/CollapsibleTitledPanel.h"
#include "View/LayerEditor.h"
#include "View/ModEditor.h"
#include "View/TitledPanel.h"
#include "View/ViewConstants.h"

#include <wx/notebook.h>
#include <wx/settings.h>
#include <wx/sizer.h>

namespace TrenchBroom {
    namespace View {
        MapInspector::MapInspector(QWidget* parent, MapDocumentWPtr document, GLContextManager& contextManager) :
        TabBookPage(parent) {
#if defined __APPLE__
            SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
            createGui(document, contextManager);
        }

        void MapInspector::createGui(MapDocumentWPtr document, GLContextManager& contextManager) {
            auto* sizer = new QVBoxLayout();
            sizer->addWidget(createLayerEditor(this, document), 1, wxEXPAND);
            sizer->addWidget(new BorderLine(this, BorderLine::Direction_Horizontal), 0, wxEXPAND);
            sizer->addWidget(createModEditor(this, document), 0, wxEXPAND);
            SetSizer(sizer);
        }

        QWidget* MapInspector::createLayerEditor(QWidget* parent, MapDocumentWPtr document) {
            TitledPanel* titledPanel = new TitledPanel(parent, "Layers");
            LayerEditor* layerEditor = new LayerEditor(titledPanel->getPanel(), document);
            
            auto* sizer = new QVBoxLayout();
            sizer->addWidget(layerEditor, 1, wxEXPAND);
            titledPanel->getPanel()->SetSizer(sizer);
            
            return titledPanel;
        }

        QWidget* MapInspector::createModEditor(QWidget* parent, MapDocumentWPtr document) {
            CollapsibleTitledPanel* titledPanel = new CollapsibleTitledPanel(parent, "Mods", false);
            ModEditor* modEditor = new ModEditor(titledPanel->getPanel(), document);

            auto* sizer = new QVBoxLayout();
            sizer->addWidget(modEditor, 1, wxEXPAND);
            titledPanel->getPanel()->setLayout(sizer);
            
            return titledPanel;
        }
    }
}
