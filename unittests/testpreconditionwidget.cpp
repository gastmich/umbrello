/*
    Copyright 2019  Ralf Habacker  <ralf.habacker@freenet.de>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "testpreconditionwidget.h"
#include "preconditionwidget.h"

#include "folder.h"
#include "objectwidget.h"
#include "umlscene.h"

typedef TestWidget<PreconditionWidget, ObjectWidget*> TestPreconditionWidgetClass;

void TestPreconditionWidget::test_saveAndLoad()
{
    UMLFolder folder("testfolder");
    UMLScene scene(&folder);
    UMLObject o(nullptr);
    ObjectWidget ow(&scene, &o);
    scene.addWidgetCmd(&ow);
    TestPreconditionWidgetClass pw1(&scene, &ow);
    scene.addWidgetCmd(&pw1);
    QDomDocument save = pw1.testSave1();
    //pw1.testDump("save");
    TestPreconditionWidgetClass pw2(&scene, nullptr);
    QCOMPARE(pw2.testLoad1(save), true);
    QCOMPARE(pw2.testSave1().toString(), save.toString());
    //pw2.testDump("load");
    QCOMPARE(pw2.objectWidget(), &ow);
}

QTEST_MAIN(TestPreconditionWidget)

