/*
 * Common Public Attribution License Version 1.0. 
 * 
 * The contents of this file are subject to the Common Public Attribution 
 * License Version 1.0 (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License 
 * at http://www.xTuple.com/CPAL.  The License is based on the Mozilla 
 * Public License Version 1.1 but Sections 14 and 15 have been added to 
 * cover use of software over a computer network and provide for limited 
 * attribution for the Original Developer. In addition, Exhibit A has 
 * been modified to be consistent with Exhibit B.
 * 
 * Software distributed under the License is distributed on an "AS IS" 
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
 * the License for the specific language governing rights and limitations 
 * under the License. 
 * 
 * The Original Code is xTuple ERP: PostBooks Edition 
 * 
 * The Original Developer is not the Initial Developer and is __________. 
 * If left blank, the Original Developer is the Initial Developer. 
 * The Initial Developer of the Original Code is OpenMFG, LLC, 
 * d/b/a xTuple. All portions of the code written by xTuple are Copyright 
 * (c) 1999-2008 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved. 
 * 
 * Contributor(s): ______________________.
 * 
 * Alternatively, the contents of this file may be used under the terms 
 * of the xTuple End-User License Agreeement (the xTuple License), in which 
 * case the provisions of the xTuple License are applicable instead of 
 * those above.  If you wish to allow use of your version of this file only 
 * under the terms of the xTuple License and not to allow others to use 
 * your version of this file under the CPAL, indicate your decision by 
 * deleting the provisions above and replace them with the notice and other 
 * provisions required by the xTuple License. If you do not delete the 
 * provisions above, a recipient may use your version of this file under 
 * either the CPAL or the xTuple License.
 * 
 * EXHIBIT B.  Attribution Information
 * 
 * Attribution Copyright Notice: 
 * Copyright (c) 1999-2008 by OpenMFG, LLC, d/b/a xTuple
 * 
 * Attribution Phrase: 
 * Powered by xTuple ERP: PostBooks Edition
 * 
 * Attribution URL: www.xtuple.org 
 * (to be included in the "Community" menu of the application if possible)
 * 
 * Graphic Image as provided in the Covered Code, if any. 
 * (online at www.xtuple.com/poweredby)
 * 
 * Display of Attribution Information is required in Larger Works which 
 * are defined in the CPAL as a work which combines Covered Code or 
 * portions thereof with code not governed by the terms of the CPAL.
 */

#include "salesRep.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

#define DEBUG false

salesRep::salesRep(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_number, SIGNAL(lostFocus()), this, SLOT(sCheck()));

  _commPrcnt->setValidator(omfgThis->percentVal());
}

salesRep::~salesRep()
{
  // no need to delete child widgets, Qt does it all for us
}

void salesRep::languageChange()
{
  retranslateUi(this);
}

enum SetResponse salesRep::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("emp_id", &valid);
  if (valid)
  {
    _employee->setId(param.toInt());
    _employee->setEnabled(false);
    _number->setText(_employee->number());
    _number->setEnabled(false);
    _active->setFocus();
    if (DEBUG)
      qDebug("salesRep::set() got emp_id %d with number %s",
             param.toInt(), qPrintable(_employee->number()));
  }

  param = pParams.value("salesrep_id", &valid);
  if (valid)
  {
    _salesrepid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _number->setFocus();
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;
      _name->setFocus();
    }
    else if (param.toString() == "view")
    {
      _mode = cView;
      _number->setEnabled(FALSE);
      _name->setEnabled(FALSE);
      _active->setEnabled(FALSE);
      _commPrcnt->setEnabled(FALSE);
      _employee->setEnabled(FALSE);
      _close->setText(tr("&Close"));
      _save->hide();
      _close->setFocus();
    }
  }

  return NoError;
}

void salesRep::sCheck()
{
  _number->setText(_number->text().stripWhiteSpace());
  if ((_mode == cNew) && (_number->text().length()))
  {
    q.prepare( "SELECT salesrep_id "
               "FROM salesrep "
               "WHERE (UPPER(salesrep_number)=UPPER(:salesrep_number));" );
    q.bindValue(":salesrep_number", _number->text());
    q.exec();
    if (q.first())
    {
      _salesrepid = q.value("salesrep_id").toInt();
      _mode = cEdit;
      populate();

      _number->setEnabled(FALSE);
    }
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void salesRep::sSave()
{
  if (_number->text().stripWhiteSpace().length() == 0)
  {
    QMessageBox::critical( this, tr("Cannot Save Sales Rep."),
                           tr("You must enter a Number for this Sales Rep.") );
    _number->setFocus();
    return;
  }

  if (_commPrcnt->text().stripWhiteSpace().length() == 0)
  {
    QMessageBox::critical( this, tr("Cannot Save Sales Rep."),
                           tr("You must enter a Commission Rate for this Sales Rep.") );
    _commPrcnt->setFocus();
    return;
  }

  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('salesrep_salesrep_id_seq') AS salesrep_id;");
    if (q.first())
      _salesrepid = q.value("salesrep_id").toInt();
    else if (q.lastError().type() != QSqlError::None)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
 
    q.prepare( "INSERT INTO salesrep "
               "(salesrep_id, salesrep_number, salesrep_active, salesrep_name, salesrep_commission) "
               "VALUES "
               "(:salesrep_id, :salesrep_number, :salesrep_active, :salesrep_name, :salesrep_commission);" );
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE salesrep "
               "SET salesrep_active=:salesrep_active, salesrep_number=:salesrep_number,"
               "    salesrep_name=:salesrep_name, salesrep_commission=:salesrep_commission "
               "WHERE (salesrep_id=:salesrep_id);" );

  q.bindValue(":salesrep_id", _salesrepid);
  q.bindValue(":salesrep_number", _number->text());
  q.bindValue(":salesrep_name", _name->text());
  q.bindValue(":salesrep_commission", (_commPrcnt->toDouble() / 100));
  q.bindValue(":salesrep_active", QVariant(_active->isChecked(), 0));
  q.exec();
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  done(_salesrepid);
}

void salesRep::populate()
{
  q.prepare( "SELECT salesrep_number, salesrep_active, salesrep_name,"
             "       salesrep_commission "
             "FROM salesrep "
             "WHERE (salesrep_id=:salesrep_id);" );
  q.bindValue(":salesrep_id", _salesrepid);
  q.exec();
  if (q.first())
  {
    _number->setText(q.value("salesrep_number").toString());
    _active->setChecked(q.value("salesrep_active").toBool());
    _name->setText(q.value("salesrep_name").toString());
    _commPrcnt->setDouble(q.value("salesrep_commission").toDouble() * 100);
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
