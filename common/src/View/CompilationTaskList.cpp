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

#include "CompilationTaskList.h"

#include "EL/Interpolator.h"
#include "Model/CompilationProfile.h"
#include "View/AutoCompleteTextControl.h"
#include "View/ELAutoCompleteHelper.h"
#include "View/BorderLine.h"
#include "View/CompilationVariables.h"
#include "View/TitledPanel.h"
#include "View/ViewConstants.h"

#include <QBoxLayout>
#include <QCompleter>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringListModel>

namespace TrenchBroom {
    namespace View {
        CompilationTaskEditorBase::CompilationTaskEditorBase(const QString& title, MapDocumentWPtr document, Model::CompilationProfile& profile, Model::CompilationTask& task, QWidget* parent) :
        ControlListBoxItemRenderer(parent),
        m_title(title),
        m_document(document),
        m_profile(&profile),
        m_task(&task),
        m_panel(nullptr) {
            m_panel = new TitledPanel(this, m_title);

            auto* layout = new QVBoxLayout();
            layout->addWidget(m_panel);
            setLayout(layout);

            addProfileObservers();
            addTaskObservers();
        }

        CompilationTaskEditorBase::~CompilationTaskEditorBase() {
            removeProfileObservers();
            removeTaskObservers();
        }

        void CompilationTaskEditorBase::setupAutoCompletion(QLineEdit* lineEdit) {

            auto* completer = new QCompleter();
            completer->setCaseSensitivity(Qt::CaseInsensitive);
            lineEdit->setCompleter(completer);

            auto* model = new QStringListModel();
            m_completerModels.push_back(model);
            updateAutoComplete(model);
        }

        void CompilationTaskEditorBase::updateAutoComplete(QStringListModel* model) {
            const auto workDir = EL::interpolate(m_profile->workDirSpec(), CompilationWorkDirVariables(lock(m_document)));
            const auto variables = CompilationVariables(lock(m_document), workDir);

            auto completions = QStringList();
            for (const auto& name : variables.names()) {
                completions.append(QString::fromStdString(name));
            }

            model->setStringList(completions);
        }

        void CompilationTaskEditorBase::addProfileObservers() {
            m_profile->profileWillBeRemoved.addObserver(this, &CompilationTaskEditorBase::profileWillBeRemoved);
            m_profile->profileDidChange.addObserver(this, &CompilationTaskEditorBase::profileDidChange);
        }

        void CompilationTaskEditorBase::removeProfileObservers() {
            if (m_profile != nullptr) {
                m_profile->profileWillBeRemoved.removeObserver(this, &CompilationTaskEditorBase::profileWillBeRemoved);
                m_profile->profileDidChange.removeObserver(this, &CompilationTaskEditorBase::profileDidChange);
            }
        }

        void CompilationTaskEditorBase::addTaskObservers() {
            m_task->taskWillBeRemoved.addObserver(this, &CompilationTaskEditorBase::taskWillBeRemoved);
            m_task->taskDidChange.addObserver(this, &CompilationTaskEditorBase::taskDidChange);
        }

        void CompilationTaskEditorBase::removeTaskObservers() {
            if (m_task != nullptr) {
                m_task->taskWillBeRemoved.removeObserver(this, &CompilationTaskEditorBase::taskWillBeRemoved);
                m_task->taskDidChange.removeObserver(this, &CompilationTaskEditorBase::taskDidChange);
            }
        }

        void CompilationTaskEditorBase::profileWillBeRemoved() {
            removeProfileObservers();
            removeTaskObservers();
            m_task = nullptr;
            m_profile = nullptr;
        }

        void CompilationTaskEditorBase::profileDidChange() {
            for (auto* model : m_completerModels) {
                updateAutoComplete(model);
            }
        }

        void CompilationTaskEditorBase::taskWillBeRemoved() {
            removeTaskObservers();
            m_task = nullptr;
        }

        void CompilationTaskEditorBase::taskDidChange() {
            if (m_task != nullptr) {
                updateTask();
            }
        }

        void CompilationTaskEditorBase::update(const size_t index) {
            updateTask();
        }

        CompilationExportMapTaskEditor::CompilationExportMapTaskEditor(MapDocumentWPtr document, Model::CompilationProfile& profile, Model::CompilationExportMap& task, QWidget* parent) :
        CompilationTaskEditorBase("Export Map", document, profile, task, parent),
        m_targetEditor(nullptr) {
            auto* formLayout = new QFormLayout();
            formLayout->setContentsMargins(LayoutConstants::WideHMargin, LayoutConstants::WideVMargin, LayoutConstants::WideHMargin, LayoutConstants::WideVMargin);
            formLayout->setVerticalSpacing(LayoutConstants::NarrowVMargin);
            formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
            m_panel->getPanel()->setLayout(formLayout);

            m_targetEditor = new QLineEdit();
            setupAutoCompletion(m_targetEditor);
            formLayout->addRow("Target", m_targetEditor);

            connect(m_targetEditor, &QLineEdit::textEdited, this, &CompilationExportMapTaskEditor::targetSpecChanged);
        }

        void CompilationExportMapTaskEditor::updateTask() {
            const auto targetSpec = QString::fromStdString(task().targetSpec());
            if (m_targetEditor->text() != targetSpec) {
                m_targetEditor->setText(targetSpec);
            }
        }

        Model::CompilationExportMap& CompilationExportMapTaskEditor::task() {
            // This is safe because we know what type of task the editor was initialized with.
            // We have to do this to avoid using a template as the base class.
            return static_cast<Model::CompilationExportMap&>(*m_task);
        }

        void CompilationExportMapTaskEditor::targetSpecChanged(const QString& text) {
            task().setTargetSpec(text.toStdString());
        }

        CompilationCopyFilesTaskEditor::CompilationCopyFilesTaskEditor(MapDocumentWPtr document, Model::CompilationProfile& profile, Model::CompilationCopyFiles& task, QWidget* parent) :
        CompilationTaskEditorBase("Copy Files", document, profile, task, parent),
        m_sourceEditor(nullptr),
        m_targetEditor(nullptr) {
            auto* formLayout = new QFormLayout();
            formLayout->setContentsMargins(LayoutConstants::WideHMargin, LayoutConstants::WideVMargin, LayoutConstants::WideHMargin, LayoutConstants::WideVMargin);
            formLayout->setVerticalSpacing(LayoutConstants::NarrowVMargin);
            formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
            m_panel->getPanel()->setLayout(formLayout);

            m_sourceEditor = new QLineEdit();
            setupAutoCompletion(m_sourceEditor);
            formLayout->addRow("Source", m_sourceEditor);

            m_targetEditor = new QLineEdit();
            setupAutoCompletion(m_targetEditor);
            formLayout->addRow("Target", m_targetEditor);

            connect(m_sourceEditor, &QLineEdit::textEdited, this, &CompilationCopyFilesTaskEditor::sourceSpecChanged);
            connect(m_targetEditor, &QLineEdit::textEdited, this, &CompilationCopyFilesTaskEditor::targetSpecChanged);
        }

        void CompilationCopyFilesTaskEditor::updateTask() {
            const auto sourceSpec = QString::fromStdString(task().sourceSpec());
            if (m_sourceEditor->text() != sourceSpec) {
                m_sourceEditor->setText(sourceSpec);
            }

            const auto targetSpec = QString::fromStdString(task().targetSpec());
            if (m_targetEditor->text() != targetSpec) {
                m_targetEditor->setText(targetSpec);
            }
        }

        Model::CompilationCopyFiles& CompilationCopyFilesTaskEditor::task() {
            // This is safe because we know what type of task the editor was initialized with.
            // We have to do this to avoid using a template as the base class.
            return static_cast<Model::CompilationCopyFiles&>(*m_task);
        }

        void CompilationCopyFilesTaskEditor::sourceSpecChanged(const QString& text) {
            task().setSourceSpec(text.toStdString());
        }

        void CompilationCopyFilesTaskEditor::targetSpecChanged(const QString& text) {
            task().setTargetSpec(text.toStdString());
        }

        CompilationRunToolTaskEditor::CompilationRunToolTaskEditor(MapDocumentWPtr document, Model::CompilationProfile& profile, Model::CompilationRunTool& task, QWidget* parent) :
        CompilationTaskEditorBase("Run Tool", document, profile, task, parent),
        m_toolEditor(nullptr),
        m_parametersEditor(nullptr) {
            auto* formLayout = new QFormLayout();
            formLayout->setContentsMargins(LayoutConstants::WideHMargin, LayoutConstants::WideVMargin, LayoutConstants::WideHMargin, LayoutConstants::WideVMargin);
            formLayout->setVerticalSpacing(LayoutConstants::NarrowVMargin);
            formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
            m_panel->getPanel()->setLayout(formLayout);

            m_toolEditor = new QLineEdit();
            setupAutoCompletion(m_toolEditor);

            auto* browseToolButton = new QPushButton("...");
            browseToolButton->setToolTip("Click to browse");

            auto* toolLayout = new QHBoxLayout();
            toolLayout->setContentsMargins(0, 0, 0, 0);
            toolLayout->setSpacing(LayoutConstants::NarrowHMargin);
            toolLayout->addWidget(m_toolEditor, 1);
            toolLayout->addWidget(browseToolButton);

            formLayout->addRow("Tool", toolLayout);

            m_parametersEditor = new QLineEdit();
            setupAutoCompletion(m_parametersEditor);
            formLayout->addRow("Parameters", m_parametersEditor);

            connect(m_toolEditor, &QLineEdit::textEdited, this, &CompilationRunToolTaskEditor::toolSpecChanged);
            connect(browseToolButton, &QPushButton::clicked, this, &CompilationRunToolTaskEditor::browseTool);
            connect(m_parametersEditor, &QLineEdit::textEdited, this, &CompilationRunToolTaskEditor::parameterSpecChanged);
        }

        void CompilationRunToolTaskEditor::updateTask() {
            const auto toolSpec = QString::fromStdString(task().toolSpec());
            if (m_toolEditor->text() != toolSpec) {
                m_toolEditor->setText(toolSpec);
            }

            const auto parametersSpec = QString::fromStdString(task().parameterSpec());
            if (m_parametersEditor->text() != parametersSpec) {
                m_parametersEditor->setText(parametersSpec);
            }
        }

        Model::CompilationRunTool& CompilationRunToolTaskEditor::task() {
            // This is safe because we know what type of task the editor was initialized with.
            // We have to do this to avoid using a template as the base class.
            return static_cast<Model::CompilationRunTool&>(*m_task);
        }

        void CompilationRunToolTaskEditor::browseTool() {
            const QString fileName = QFileDialog::getOpenFileName(this, "Select Tool");
            if (!fileName.isEmpty()) {
                task().setToolSpec(fileName.toStdString());
            }
        }

        void CompilationRunToolTaskEditor::toolSpecChanged(const QString& text) {
            task().setToolSpec(m_toolEditor->text().toStdString());
        }

        void CompilationRunToolTaskEditor::parameterSpecChanged(const QString& text) {
            task().setParameterSpec(m_parametersEditor->text().toStdString());
        }

        CompilationTaskList::CompilationTaskList(MapDocumentWPtr document, QWidget* parent) :
        ControlListBox("Click the '+' button to create a task.", parent),
        m_document(document),
        m_profile(nullptr) {}

        CompilationTaskList::~CompilationTaskList() {
            if (m_profile != nullptr) {
                m_profile->profileDidChange.removeObserver(this, &CompilationTaskList::profileDidChange);
            }
        }

        void CompilationTaskList::setProfile(Model::CompilationProfile* profile) {
            if (m_profile != nullptr) {
                m_profile->profileDidChange.removeObserver(this, &CompilationTaskList::profileDidChange);
            }
            m_profile = profile;
            if (m_profile != nullptr) {
                m_profile->profileDidChange.addObserver(this, &CompilationTaskList::profileDidChange);
            }
            reload();
        }

        void CompilationTaskList::profileDidChange() {
            reload();
        }

        class CompilationTaskList::CompilationTaskEditorFactory : public Model::CompilationTaskVisitor {
        private:
            MapDocumentWPtr m_document;
            Model::CompilationProfile& m_profile;
            QWidget* m_parent;
            ControlListBoxItemRenderer* m_result;
        public:
            CompilationTaskEditorFactory(MapDocumentWPtr document, Model::CompilationProfile& profile, QWidget* parent) :
            m_document(document),
            m_profile(profile),
            m_parent(parent),
            m_result(nullptr) {}

            ControlListBoxItemRenderer* result() const {
                return m_result;
            }

            void visit(Model::CompilationExportMap& task) override {
                m_result = new CompilationExportMapTaskEditor(m_document, m_profile, task, m_parent);
            }

            void visit(Model::CompilationCopyFiles& task) override {
                m_result = new CompilationCopyFilesTaskEditor(m_document, m_profile, task, m_parent);
            }

            void visit(Model::CompilationRunTool& task) override {
                m_result = new CompilationRunToolTaskEditor(m_document, m_profile, task, m_parent);
            }
        };

        size_t CompilationTaskList::itemCount() const {
            return m_profile->taskCount();
        }

        ControlListBoxItemRenderer* CompilationTaskList::createItemRenderer(QWidget* parent, const size_t index) {
            ensure(m_profile != nullptr, "profile is null");

            CompilationTaskEditorFactory factory(m_document, *m_profile, parent);
            auto* task = m_profile->task(index);
            task->accept(factory);
            return factory.result();
        }
    }
}
