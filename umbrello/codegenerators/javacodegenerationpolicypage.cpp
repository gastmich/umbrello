/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2004-2006                                               *
 *   Umbrello UML Modeller Authors <uml-devel@ uml.sf.net>                 *
 ***************************************************************************/

/*  This code generated by:
 *      Author : thomas
 *      Date   : Wed Jul 30 2003
 */

// own header
#include "javacodegenerationpolicypage.h"
// qt/kde includes
#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <kdebug.h>
#include <klocale.h>
// app includes
#include "javacodegenerator.h"
#include "../uml.h"

JavaCodeGenerationPolicyPage::JavaCodeGenerationPolicyPage( QWidget *parent, const char *name, JavaCodeGenerationPolicy * policy )
  : CodeGenerationPolicyPage(parent, name, UMLApp::app()->getCommonPolicy())
{
    CodeGenerationPolicy *commonPolicy = UMLApp::app()->getCommonPolicy();
    form = new JavaCodeGenerationFormBase(this);
    form->m_SelectCommentStyle->setCurrentItem((int)(commonPolicy->getCommentStyle()));
    form->m_generateConstructors->setChecked(commonPolicy->getAutoGenerateConstructors());
    form->m_generateAttribAccessors->setChecked(policy->getAutoGenerateAttribAccessors());
    form->m_generateAssocAccessors->setChecked(policy->getAutoGenerateAssocAccessors());
    form->m_accessorScopeCB->setCurrentItem(commonPolicy->getAttributeAccessorScope() - 200);
    form->m_assocFieldScopeCB->setCurrentItem(commonPolicy->getAssociationFieldScope() - 200);

    CodeGenerator *codegen = UMLApp::app()->getGenerator();
    JavaCodeGenerator *javacodegen = dynamic_cast<JavaCodeGenerator*>(codegen);
    // @todo unclean - CreateANTBuildFile attribute should be in java policy
    if (javacodegen)
        form->m_makeANTDocumentCheckBox->setChecked(javacodegen->getCreateANTBuildFile());
}

JavaCodeGenerationPolicyPage::~JavaCodeGenerationPolicyPage()
{
}

void JavaCodeGenerationPolicyPage::apply()
{
    CodeGenerationPolicy *commonPolicy = UMLApp::app()->getCommonPolicy();
    JavaCodeGenerationPolicy * parent = (JavaCodeGenerationPolicy*) m_parentPolicy;

    // block signals so we don't cause too many update content calls to code documents
    commonPolicy->blockSignals(true);

    commonPolicy->setCommentStyle((CodeGenerationPolicy::CommentStyle ) form->m_SelectCommentStyle->currentItem());
    commonPolicy->setAttributeAccessorScope((CodeGenerationPolicy::ScopePolicy) (form->m_accessorScopeCB->currentItem()+200));
    commonPolicy->setAssociationFieldScope((CodeGenerationPolicy::ScopePolicy) (form->m_assocFieldScopeCB->currentItem()+200));
    commonPolicy->setAutoGenerateConstructors(form->m_generateConstructors->isChecked());
    parent->setAutoGenerateAttribAccessors(form->m_generateAttribAccessors->isChecked());
    parent->setAutoGenerateAssocAccessors(form->m_generateAssocAccessors->isChecked());

    CodeGenerator *codegen = UMLApp::app()->getGenerator();
    JavaCodeGenerator *javacodegen = dynamic_cast<JavaCodeGenerator*>(codegen);
    if (javacodegen)
        javacodegen->setCreateANTBuildFile(form->m_makeANTDocumentCheckBox->isChecked());

    commonPolicy->blockSignals(false);

    // now send out modified code content signal
    commonPolicy->emitModifiedCodeContentSig();

}


#include "javacodegenerationpolicypage.moc"
