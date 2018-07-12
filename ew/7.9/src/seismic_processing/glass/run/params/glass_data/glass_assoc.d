Cut 9 50.0 # Empirical Params
TimeRange -600.0 500.0 -820.0
TimeStep 5.0

# Shells
Shell	  5.0
Shell	 20.0
Shell	 60.0
Shell	100.0
Shell	200.0
Shell	400.0
Shell	660.0


# Glass State - Transition Rules
Tran	Unassigned_Pick			1	Assign		100
Tran	Unassignable_Pick			1	Associate		100

Tran	Changed_Origin			1	Locate		100
Tran	Origin_w_Changed_Assocs		1	Locate		100
Tran	Located_Origin			1	Prune			100
Tran	Located_Origin			1	Focus			 50
Tran	Located_Origin			1	Waif			100
Tran	Located_Origin			1	Scavenge		100
Tran	Stabilized_Origin			1	Validate_Origin	100
Tran	Validated_Origin			1	Update_GUI		100
Tran	Invalidated_Origin		1	Delete		100
Tran	Deleted_Origin			1	Update_GUI		100

# Uncomment the next lines for more GUI updates / slower performance
#  Not currently supported
#Tran	STATE_PICK_ASSOCIATED	1	Update_GUI		100
#Tran	Unassociable_Pick		1	Update_GUI		100


## Old
#Tran	Assign		0	Associate	100
#Tran	Locate		1	Focus		100
#Tran	Locate		1	Waif		20
#Tran	Locate		1	Prune		100
#Tran	Locate		1	Scavenge	20
##Tran	Focus		1	Locate		100
#Tran	Waif		1	Locate		100
#Tran	Prune		1	Locate		100
#Tran	Scavenge	1	Locate		100
#Tran	Assign		1	Locate		100
#Tran	Associate	1	Locate		100
#Tran	Scavenged	1	Locate		100

