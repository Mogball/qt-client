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

#include "voucherMiscDistrib.h"

#include <QVariant>
#include <QMessageBox>
#include <QValidator>
/*
 *  Constructs a voucherMiscDistrib as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
voucherMiscDistrib::voucherMiscDistrib(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);


  // signals and slots connections
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
voucherMiscDistrib::~voucherMiscDistrib()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void voucherMiscDistrib::languageChange()
{
  retranslateUi(this);
}

enum SetResponse voucherMiscDistrib::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("vohead_id", &valid);
  if (valid)
    _voheadid = param.toInt();

  param = pParams.value("curr_id", &valid);
  if (valid)
    _amount->setId(param.toInt());

  param = pParams.value("curr_effective", &valid);
  if (valid)
    _amount->setEffective(param.toDate());

  param = pParams.value("vodist_id", &valid);
  if (valid)
  {
    _vodistid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      param = pParams.value("amount", &valid);
      if (valid)
        _amount->setLocalValue(param.toDouble());
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      _amount->setFocus();
    }
  }

  return NoError;
}

void voucherMiscDistrib::populate()
{
  q.prepare( "SELECT vodist_accnt_id,"
             "       vodist_amount, "
             "       vodist_expcat_id "
             "FROM vodist "
             "WHERE (vodist_id=:vodist_id);" ) ;
  q.bindValue(":vodist_id", _vodistid);
  q.exec();
  if (q.first())
  {
    _account->setId(q.value("vodist_accnt_id").toInt());
    _amount->setLocalValue(q.value("vodist_amount").toDouble());
    if(q.value("vodist_expcat_id").toInt() != -1)
    {
      _expcatSelected->setChecked(TRUE);
      _expcat->setId(q.value("vodist_expcat_id").toInt());
    }
  }
}

void voucherMiscDistrib::sSave()
{
  if (_accountSelected->isChecked() && !_account->isValid())
  {
    QMessageBox::warning( this, tr("Select Account"),
                          tr("You must select an Account to post this Miscellaneous Distribution to.") );
    _account->setFocus();
    return;
  }

  if (_expcatSelected->isChecked() && !_expcat->isValid())
  {
    QMessageBox::warning( this, tr("Select Expense Category"),
                          tr("You must select an Expense Category to post this Miscellaneous Distribution to.") );
    _account->setFocus();
    return;
  }

  if (_amount->isZero())
  {
    QMessageBox::warning( this, tr("Enter Amount"),
                          tr("You must enter an amount for this Miscellaneous Distribution.") );
    _amount->setFocus();
    return;
  }

  if (_mode == cNew)
  {
    q.exec("SELECT NEXTVAL('vodist_vodist_id_seq') AS _vodistid;");
    if (q.first())
      _vodistid = q.value("_vodistid").toInt();
    else
    {
      systemError(this, tr("A System Error occurred at %1::%2.")
                        .arg(__FILE__)
                        .arg(__LINE__) );
      return;
    }

    q.prepare( "INSERT INTO vodist "
               "( vodist_id, vodist_vohead_id, vodist_poitem_id,"
               "  vodist_costelem_id, vodist_accnt_id, vodist_amount, vodist_expcat_id ) "
               "VALUES "
               "( :vodist_id, :vodist_vohead_id, -1,"
               "  -1, :vodist_accnt_id, :vodist_amount, :vodist_expcat_id );" );
  }
  else if (_mode == cEdit)
    q.prepare( "UPDATE vodist "
               "SET vodist_accnt_id=:vodist_accnt_id,"
               "    vodist_amount=:vodist_amount,"
               "    vodist_expcat_id=:vodist_expcat_id "
               "WHERE (vodist_id=:vodist_id);" );
  
  q.bindValue(":vodist_id", _vodistid);
  q.bindValue(":vodist_vohead_id", _voheadid);
  q.bindValue(":vodist_amount", _amount->localValue());
  if(_accountSelected->isChecked())
  {
    q.bindValue(":vodist_accnt_id", _account->id());
    q.bindValue(":vodist_expcat_id", -1);
  }
  else
  {
    q.bindValue(":vodist_accnt_id", -1);
    q.bindValue(":vodist_expcat_id", _expcat->id());
  }
  q.exec();

  done(_vodistid);
}

