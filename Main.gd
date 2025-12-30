extends Node3D

@onready var car = $Car
var start_transform : Transform3D
var time_elapsed = 0.0

func _ready():
	start_transform = car.global_transform

func _process(delta):
	time_elapsed += delta
	$CanvasLayer/Label.text = "Time: %.2f" % time_elapsed

	if Input.is_action_just_pressed("ui_cancel"):
		reset_car()

func reset_car():
	car.linear_velocity = Vector3.ZERO
	car.angular_velocity = Vector3.ZERO
	car.global_transform = start_transform
	time_elapsed = 0.0

func _on_reset_button_pressed():
	reset_car()
