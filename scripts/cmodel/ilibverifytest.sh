SCRIPT="./build/test/tools/ilibverifytest"

# Tests `ilibverifytest $1 $2 [$3]` for $1 in {assert,require} and $2 in {0,1}.
function test_assertion() {
	OP="$1"
	COND="$2"
	MSG="$3"

	cmd="${SCRIPT} ${OP} ${COND} ${MSG}"
	res=$(${cmd} 2>&1)
	rc=$?

	if [ "${OP}" == "assert" ] && [ "${rc}" -eq "${COND}" ]; then
		echo "${cmd}: Return code does not match condition."
	elif [ "${OP}" == "require" ] && [ "${rc}" -eq 1 ]; then
		echo "${cmd}: Non-zero return code on assume."
	elif [ "${COND}" -eq 1 ]; then
		if [ ! -z ${res} ]; then
			echo "${cmd}: Message on success."
		fi
	elif [ "${COND}" -eq 0 ]; then
		if [ -z "${MSG}" ] && [ "${res}" != "${OP}" ]; then
			echo "${cmd}: Custom message when unprovided."
		fi
	fi
}

function test_nd() {
	TYPE="$1"
	VAL="$2"
	MSG="placeholder_msg"

	cmd="${SCRIPT} nd ${TYPE} ${MSG}"
	res=$(${cmd} 2>&1 <<< ${val})
	rc=$?

	if [ "${rc}" != "${VAL}" ]; then
		echo "${cmd}: Incorrect nd value returned."
	fi

	grep "${MSG}" >/dev/null 2>&1 <<< "${res}"
	MESSAGE_SET=$?
	if [ ${MESSAGE_SET} -ne 0 ]; then
		echo "${cmd}: Message did not appear in promt."
	fi
}

for op in "assert" "require"; do
	for cond in 0 1; do
		for msg in "" "Message"; do
			res=$(test_assertion "${op}" "${cond}" "${msg}")
			if [ ! -z "${res}" ]; then
				echo "${res}"
			fi
		done
	done
done

for type in $(seq 0 11); do
	for val in $(seq 0 10); do
		res=$(test_nd "${type}" "${val}")
		if [ ! -z "${res}" ]; then
			echo "${res}"
		fi
	done
done

