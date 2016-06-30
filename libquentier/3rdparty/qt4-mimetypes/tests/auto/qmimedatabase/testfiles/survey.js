// Out of error numbers? Fear not, Wikipedia (via TinyURL) to the
// rescue! http://tinyurl.com/lf9q7

function checkOther(the_name) {
	var other = document.getElementById(the_name + '_99')
	var specify = document.getElementById(the_name + '_other')
	
	if (!other) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-3141-"+document.forms[0].__page.value+".\n\nThank you for your help."); // π
		return
	}

	if (!specify) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-2718-"+document.forms[0].__page.value+".\n\nThank you for your help."); // e
		return
	}

	if (other.checked) {
		specify.disabled = false;
	} else {
		specify.disabled = true;
	}
}


function checkBoxOther(the_name) {
	var other = document.getElementById(the_name + '_other_check_1');
	var specify = document.getElementById(the_name + '_other_specify');

	if (!other) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-0577-"+document.forms[0].__page.value+".\n\nThank you for your help."); // γ
		return
	}

	if (!specify) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-0702-"+document.forms[0].__page.value+".\n\nThank you for your help."); // β*
		return
	}

	if (other.checked && !other.disabled) {
		specify.disabled = false;
	} else {
		specify.disabled = true;
	}
}


function checkBoxOtherNone(the_name) {
	checkBoxNone(the_name)
	checkBoxOther(the_name)
}

function checkBoxNone(the_name) {
	var none = document.getElementById(the_name + '_none_1')

	if (!none) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-4669-"+document.forms[0].__page.value+".\n\nThank you for your help."); // δ
		return
	}

	// find the enclosing ul
	var ul = none;
	do { ul = ul.parentNode } while (ul && 'UL' != ul.tagName) 
	
	if (!ul) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-2502-"+document.forms[0].__page.value+".\n\nThank you for your help."); // α
		return
	}

	var inputs = ul.getElementsByTagName('input')
	var disable = none.checked
	for (var i = 0; i < inputs.length; ++i) {
		if ('checkbox' == inputs[i].type.toLowerCase() && inputs[i] != none)
			inputs[i].disabled = disable;
	}
}

function updateSum(theName) {
	var holder = document.getElementById(theName+'_holder');
	if (!holder) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-1414-"+document.forms[0].__page.value+".\n\nThank you for your help."); // √2
		return
	}

	var inputs = holder.getElementsByTagName('input');
	if (!inputs || inputs.length == 0) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-1732-"+document.forms[0].__page.value+".\n\nThank you for your help."); // √3
		return
	}

	var sumspot = document.getElementById(theName+'_sum');
	if (!sumspot) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-1618-"+document.forms[0].__page.value+".\n\nThank you for your help."); // φ
		return
	}

	var sum = 0.0;
	for (var i = 0; i < inputs.length; ++i) {
		sum += 1*(inputs[i].value);
	}
	
	sumspot.innerHTML = sum;
}


function regionQuestion(the_name) {
	var USbutton = document.getElementById(the_name + '_region_1')
	var CANbutton = document.getElementById(the_name + '_region_2')
	var USsel = document.getElementById(the_name + '_usa_state_sel')
	var CANsel = document.getElementById(the_name + '_can_province_sel')
	
	if (!USbutton) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-0660-"+document.forms[0].__page.value+".\n\nThank you for your help."); // C₂
		return
	}
	if (!CANbutton) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-0261-"+document.forms[0].__page.value+".\n\nThank you for your help."); // M₁
		return
	}
	if (!USsel) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-1902-"+document.forms[0].__page.value+".\n\nThank you for your help."); // B₂
		return
	}
	if (!CANsel) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-0870-"+document.forms[0].__page.value+".\n\nThank you for your help."); // B₄
		return
	}

	if (USbutton.selected && CANbutton.selected) {
		alert("Sorry, an internal error has occured. Please contact webmaster@metrics.net and alert him of this problem. Please referrence error #WMPP-0915-"+document.forms[0].__page.value+".\n\nThank you for your help."); // K₄
		return
	}


	if (USbutton.checked)
		USsel.disabled = false;
	else
		USsel.disabled = true;

	if (CANbutton.checked)
		CANsel.disabled = false;
	else
		CANsel.disabled = true;

}
