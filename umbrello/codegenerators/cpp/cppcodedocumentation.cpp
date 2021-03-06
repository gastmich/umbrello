/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2003      Brian Thomas <thomas@mail630.gsfc.nasa.gov>   *
 *   copyright (C) 2004-2020                                               *
 *   Umbrello UML Modeller Authors <umbrello-devel@kde.org>                *
 ***************************************************************************/

// own header
#include "cppcodedocumentation.h"

// app includes
#include "codedocument.h"
#include "codegenerator.h"
#include "codegenerationpolicy.h"
#include "uml.h"

// qt includes
#include <QRegExp>

CPPCodeDocumentation::CPPCodeDocumentation(CodeDocument * doc, const QString & text)
  : CodeComment(doc, text)
{
}

CPPCodeDocumentation::~CPPCodeDocumentation()
{
}

/**
 * Save the XMI representation of this object
 */
void CPPCodeDocumentation::saveToXMI1(QDomDocument & doc, QDomElement & root)
{
    QDomElement blockElement = doc.createElement(QLatin1String("cppcodedocumentation"));
    setAttributesOnNode(doc, blockElement); // as we added no additional fields to this class we may
    // just use parent TextBlock method
    root.appendChild(blockElement);
}

/**
 * @return   QString
 */
QString CPPCodeDocumentation::toString() const
{
    QString output;

    // simple output method
    if(getWriteOutText())
    {
        bool useDoubleDashOutput = true;

        // need to figure out output type from cpp policy
        CodeGenerationPolicy * p = UMLApp::app()->commonPolicy();
        if(p->getCommentStyle() == CodeGenerationPolicy::MultiLine)
            useDoubleDashOutput = false;

        QString indent = getIndentationString();
        QString endLine = getNewLineEndingChars();
        QString body = getText();
        if(useDoubleDashOutput)
        {
            if(!body.isEmpty())
                output.append(formatMultiLineText (body, indent + QLatin1String("// "), endLine));
        } else {
            output.append(indent + QLatin1String("/**") + endLine);
            output.append(formatMultiLineText (body, indent + QLatin1String(" * "), endLine));
            output.append(indent + QLatin1String(" */") + endLine);
        }
    }

    return output;
}

QString CPPCodeDocumentation::getNewEditorLine(int amount)
{
    CodeGenerationPolicy * p = UMLApp::app()->commonPolicy();
    if(p->getCommentStyle() == CodeGenerationPolicy::MultiLine)
        return getIndentationString(amount) + QLatin1String(" * ");
    else
        return getIndentationString(amount) + QLatin1String("// ");
}

int CPPCodeDocumentation::firstEditableLine()
{
    CodeGenerationPolicy * p = UMLApp::app()->commonPolicy();
    if(p->getCommentStyle() == CodeGenerationPolicy::MultiLine)
        return 1;
    return 0;
}

int CPPCodeDocumentation::lastEditableLine() 
{
    CodeGenerationPolicy * p = UMLApp::app()->commonPolicy();
    if(p->getCommentStyle() == CodeGenerationPolicy::MultiLine)
    {
        return -1; // very last line is NOT editable
    }
    return 0;
}

/** UnFormat a long text string. Typically, this means removing
 *  the indentation (linePrefix) and/or newline chars from each line.
 */
QString CPPCodeDocumentation::unformatText(const QString & text, const QString & indent)
{
    QString mytext = TextBlock::unformatText(text, indent);
    CodeGenerationPolicy * p = UMLApp::app()->commonPolicy();
    // remove leading or trailing comment stuff
    mytext.remove(QRegExp(QLatin1Char('^') + indent));
    if(p->getCommentStyle() == CodeGenerationPolicy::MultiLine)
    {
        mytext.remove(QRegExp(QLatin1String("^\\/\\*\\*\\s*\n?")));
        mytext.remove(QRegExp(QLatin1String("\\s*\\*\\/\\s*\n?$")));
        mytext.remove(QRegExp(QLatin1String("^\\s*\\*\\s*")));
    } else
        mytext.remove(QRegExp(QLatin1String("^\\/\\/\\s*")));

    return mytext;
}
