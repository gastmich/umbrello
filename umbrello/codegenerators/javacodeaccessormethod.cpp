
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*  This code generated by: 
 *      Author : thomas
 *      Date   : Fri Jun 20 2003
 */
#include <kdebug.h>

#include "javacodeaccessormethod.h"

#include "../attribute.h"
#include "../codegenerator.h"
#include "../classifiercodedocument.h"
#include "../umlobject.h"
#include "../umlrole.h"

#include "javaclassifiercodedocument.h"
#include "javacodegenerationpolicy.h"
#include "javacodeclassfield.h"
#include "javacodedocumentation.h"

// Constructors/Destructors
//  

JavaCodeAccessorMethod::JavaCodeAccessorMethod ( JavaCodeClassField * field, CodeAccessorMethod::AccessorType type)
   : CodeAccessorMethod ( (CodeClassField*) field )
{
	setType(type);

	init (field);

}

JavaCodeAccessorMethod::~JavaCodeAccessorMethod ( ) { }

// Other methods
//

void JavaCodeAccessorMethod::setAttributesOnNode ( QDomDocument & doc, QDomElement & blockElement)
{

        // set super-class attributes
        CodeAccessorMethod::setAttributesOnNode(doc, blockElement);

        // set local attributes now
}

void JavaCodeAccessorMethod::setAttributesFromNode( QDomElement & root)
{

        // set attributes from superclass method the XMI
        CodeAccessorMethod::setAttributesFromNode(root);

        // load local stuff

}

void JavaCodeAccessorMethod::updateContent( )
{

        CodeClassField * parentField = getParentClassField();
	JavaCodeClassField * javafield = (JavaCodeClassField*)parentField;
        QString fieldName = javafield->getFieldName();

        QString text = "";
        switch(getType()) {
                case CodeAccessorMethod::ADD:
		{
			int maxOccurs = javafield->maximumListOccurances();
        		JavaClassifierCodeDocument * javadoc = (JavaClassifierCodeDocument*) javafield->getParentDocument();
			QString fieldType = javafield->getTypeName();
			QString indent = getIndentation();
        		QString endLine = javadoc->getParentGenerator()->getNewLineEndingChars();
			if(maxOccurs > 0) 
				text += "if ("+fieldName+".size() < "+ QString::number(maxOccurs)+") {"+endLine+indent;
        		text += fieldName+".add(value);";
			if(maxOccurs > 0) 
			{
				text += endLine+"} else {"+endLine;
				text += indent + "System.err.println(\"ERROR: Cant add"+fieldType+" to "+fieldName+", minimum number of items reached.\");"+endLine+"}"+endLine;
			}
                        break;
		}
                case CodeAccessorMethod::GET:
                        text = "return "+fieldName+";";
                        break;
                case CodeAccessorMethod::LIST:
			text = "return (List) "+fieldName+";";
                        break;
                case CodeAccessorMethod::REMOVE:
		{
			int minOccurs = javafield->minimumListOccurances();
        		JavaClassifierCodeDocument * javadoc = (JavaClassifierCodeDocument*) javafield->getParentDocument();
			QString fieldType = javafield->getTypeName();
        		QString endLine = javadoc->getParentGenerator()->getNewLineEndingChars();
			QString indent = getIndentation();

			if(minOccurs > 0) 
				text += "if ("+fieldName+".size() >= "+ QString::number(minOccurs)+") {"+endLine+indent;
        		text += fieldName+".remove(value);";
			if(minOccurs > 0) 
			{
				text += endLine+"} else {"+endLine;
				text += indent + "System.err.println(\"ERROR: Cant remove"+fieldType+" from "+fieldName+", minimum number of items reached.\");"+endLine+"}"+endLine;
			}
                        break;
		}
                case CodeAccessorMethod::SET:
                        text = fieldName+" = value;";
                        break;
                default:
			// do nothing
                        break;
        }

        setText(text);

}

void JavaCodeAccessorMethod::updateMethodDeclaration() 
{

	JavaCodeClassField * javafield = (JavaCodeClassField*) getParentClassField();
        JavaClassifierCodeDocument * javadoc = (JavaClassifierCodeDocument*) javafield->getParentDocument();
	JavaCodeGenerationPolicy * javapolicy = (JavaCodeGenerationPolicy *) javadoc->getPolicy(); 

	// gather defs
	JavaCodeGenerationPolicy::ScopePolicy scopePolicy = javapolicy->getAttributeAccessorScope();
        QString strVis = javadoc->scopeToJavaDecl(javafield->getVisibility());
        QString fieldName = javafield->getFieldName();
        QString fieldType = javafield->getTypeName();
        QString objectType = javafield->getListObjectType();
	if(objectType.isEmpty())
		objectType = fieldName; 
        QString endLine = javadoc->getParentGenerator()->getNewLineEndingChars();

	// set scope of this accessor appropriately..if its an attribute,
	// we need to be more sophisticated
	if(javafield->parentIsAttribute())
		switch (scopePolicy) {
			case JavaCodeGenerationPolicy::Public:
			case JavaCodeGenerationPolicy::Private:
			case JavaCodeGenerationPolicy::Protected:
			strVis = javadoc->scopeToJavaDecl((Uml::Scope) scopePolicy);
				break;
			default:
			case JavaCodeGenerationPolicy::FromParent:
				// do nothing..already have taken parent value
				break;
		}

	// some variables we will need to populate
        QString headerText = ""; 
        QString methodReturnType = "";
        QString methodName = ""; 
        QString methodParams = ""; 

        switch(getType()) {
                case CodeAccessorMethod::ADD:
        		methodName = "add"+javadoc->capitalizeFirstLetter(fieldType);
                        methodReturnType = "void";
			methodParams = objectType+" value ";
			headerText = "Add an object of type "+objectType+" to the List "+fieldName+endLine+getParentObject()->getDoc()+endLine+"@return void";
                        break;
                case CodeAccessorMethod::GET:
			methodName = "get"+javadoc->capitalizeFirstLetter(fieldName);
                        methodReturnType = fieldType;
                	headerText = "Get the value of "+fieldName+endLine+getParentObject()->getDoc()+endLine+"@return the value of "+fieldName;
                        break;
                case CodeAccessorMethod::LIST:
			methodName = "get"+javadoc->capitalizeFirstLetter(fieldType)+"List";
                        methodReturnType = "List";
			headerText = "Get the list of "+fieldName+endLine+getParentObject()->getDoc()+endLine+"@return List of "+fieldName;
                        break;
                case CodeAccessorMethod::REMOVE:
			methodName = "remove"+javadoc->capitalizeFirstLetter(fieldType);
                        methodReturnType = "void";
			methodParams = objectType+" value ";
			headerText = "Remove an object of type "+objectType+" from the List "+fieldName+endLine+getParentObject()->getDoc();
                        break;
                case CodeAccessorMethod::SET:
			methodName = "set"+javadoc->capitalizeFirstLetter(fieldName);
                        methodReturnType = "void";
			methodParams = fieldType + " value ";
			headerText = "Set the value of "+fieldName+endLine+getParentObject()->getDoc()+endLine;
                        break;
		default:
			// do nothing..no idea what this is
			kdWarning()<<"Warning: cant generate JavaCodeAccessorMethod for type: "<<getType()<<endl;
                        break;
        }

        // set header once.
        if(getComment()->getText().isEmpty())
                getComment()->setText(headerText);

        // set start/end method text
        setStartMethodText(strVis+" "+methodReturnType+" "+methodName+" ( "+methodParams+" ) {");
        setEndMethodText("}");

}

void JavaCodeAccessorMethod::init ( JavaCodeClassField * field) 
{

	// lets use full-blown comment
	setComment(new JavaCodeDocumentation((JavaClassifierCodeDocument*)field->getParentDocument()));

	updateMethodDeclaration();
	updateContent();

}

#include "javacodeaccessormethod.moc"
