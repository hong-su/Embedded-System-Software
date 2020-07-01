package com.embe2020.project;


import java.io.IOException;
import android.support.v7.app.ActionBarActivity;
import android.annotation.SuppressLint;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

@SuppressLint("ShowToast")
public class MainActivity extends ActionBarActivity {

	public native void ledwrite(int dev, int value);
	public native int controlswitch();
	public native void controldotfnd(int set_num, int fnd_data);
	public native void deviceclear();
	public native void controltextlcd(String speed, String amount);
	
	int acc = 0;
	Thread_switch thread_switch;
	Thread_dot thread_dot;
	boolean running=false;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d("MainActiviry", "Start");
			
		try {
			Runtime.getRuntime().exec("su -c insmod /data/local/driver/fpga_fnd_driver.ko");
			Runtime.getRuntime().exec("su -c insmod /data/local/driver/fpga_dot_driver.ko");
			Runtime.getRuntime().exec("su -c insmod /data/local/driver/fpga_text_lcd_driver.ko");
			Runtime.getRuntime().exec("su -c insmod /data/local/driver/fpga_push_switch_driver.ko");
			
			Runtime.getRuntime().exec("su -c mknod /dev/fpga_fnd c 261 0");
			Runtime.getRuntime().exec("su -c mknod /dev/fpga_dot c 262 0");
			Runtime.getRuntime().exec("su -c mknod /dev/fpga_text_lcd c 263 0");
			Runtime.getRuntime().exec("su -c mknod /dev/fpga_push_switch c 265 0");
			
			Runtime.getRuntime().exec("su -c chmod 777 /dev/fpga_fnd");
			Runtime.getRuntime().exec("su -c chmod 777 /dev/fpga_dot");
			Runtime.getRuntime().exec("su -c chmod 777 /dev/fpga_text_lcd");
			Runtime.getRuntime().exec("su -c chmod 777 /dev/fpga_push_switch");
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		setContentView(R.layout.activity_main);
		System.loadLibrary("project");
		
		findViewById(R.id.layout).requestFocus();
		findViewById(R.id.et1).clearFocus();
		findViewById(R.id.et2).clearFocus();
		findViewById(R.id.et3).clearFocus();
		
		final Toast mytoast = Toast.makeText(this.getApplicationContext(), "Access Deny", Toast.LENGTH_SHORT);
		
		Button projload = (Button) findViewById(R.id.btprojload) ;
		projload.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	if(running){
	        		mytoast.show();
	        		return ;
	        	}
	        	running = true;
	        	
	        	thread_switch = new Thread_switch();
	        	thread_dot = new Thread_dot();
	        	
	        	thread_switch.start();
	    		thread_dot.start();
	        }
	    });
		
		Button projunload = (Button) findViewById(R.id.btprojunload) ;
		projunload.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	if(!running){
	        		mytoast.show();
	        		return ;
	        	}
	        	running = false;
	        	
	        	thread_switch.interrupt();
	    		thread_dot.interrupt();
	    		
	    		thread_switch = null;
	        	thread_dot = null;
	        	
	    		SystemClock.sleep(250);
	    		deviceclear();
	        }
	    });
		
		Button bthw1run = (Button) findViewById(R.id.bthw1run) ;
		bthw1run.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	try {
	        		Runtime.getRuntime().exec("su -c ./data/local/proj/hw1/app");
	    			 
	    		} catch (IOException e) {

	    			e.printStackTrace();
	    		}
	        }
	    });

	    Button bthw2load = (Button) findViewById(R.id.bthw2load) ;
	    bthw2load.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	try {
	        		Runtime.getRuntime().exec("su -c insmod /data/local/proj/hw2/dev_module.ko");
	        		Runtime.getRuntime().exec("su -c mknod /dev/dev_driver c 242 0");
	        		Runtime.getRuntime().exec("su -c chmod 777 /dev/dev_driver");
	    			
	    		} catch (IOException e) {
	    			e.printStackTrace();
	    		}
	        }
	    });
	    
	    Button bthw2run = (Button) findViewById(R.id.bthw2run) ;
	    bthw2run.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	EditText et1 = (EditText)findViewById(R.id.et1);
	        	EditText et2 = (EditText)findViewById(R.id.et2);
	        	EditText et3 = (EditText)findViewById(R.id.et3);
	        	String p1 = et1.getText().toString();
	        	String p2 = et2.getText().toString();
	        	String p3 = et3.getText().toString();
	        	try {
	        		Runtime.getRuntime().exec("su -c ./data/local/proj/hw2/app " + p1 + " " + p2 + " " + p3);
	        		} catch (IOException e) {
	    			e.printStackTrace();
	    		}
	        }
	    });
	    
	    Button hw2unload = (Button) findViewById(R.id.bthw2unload) ;
	    hw2unload.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	try {
	        		Runtime.getRuntime().exec("su -c rmmod /dev_module");
	        		Runtime.getRuntime().exec("su -c rm /dev/dev_driver");
	    		} catch (IOException e) {
	    			e.printStackTrace();
	    		}
	        }
	    });
	    
	    Button bthw3load = (Button) findViewById(R.id.bthw3load) ;
	    bthw3load.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	try {
	        		Runtime.getRuntime().exec("su -c insmod /data/local/proj/hw3/dev_module.ko");
	        		Runtime.getRuntime().exec("su -c mknod /dev/stopwatch c 242 0");
	        		Runtime.getRuntime().exec("su -c chmod 777 /dev/stopwatch");
	    		} catch (IOException e) {
	    			e.printStackTrace();
	    		}
	        }
	    });
	    
	    Button bthw3run = (Button) findViewById(R.id.bthw3run) ;
	    bthw3run.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	try {
	        		Runtime.getRuntime().exec("su -c ./data/local/proj/hw3/app");
	    		} catch (IOException e) {
	    			e.printStackTrace();
	    		}
	        }
	    });
	    Button hw3unload = (Button) findViewById(R.id.bthw3unload) ;
	    hw3unload.setOnClickListener(new Button.OnClickListener() {
	        @Override
	        public void onClick(View view) {
	        	try {
	        		Runtime.getRuntime().exec("su -c rmmod /dev_module");
	        		Runtime.getRuntime().exec("su -c rm /dev/stopwatch");
	    			
	    		} catch (IOException e) {
	    			e.printStackTrace();
	    		}
	        }
	    });
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
	
	@Override
	public void onBackPressed(){
		super.onBackPressed();
		this.finish();
	}
	
	@Override
	public void onDestroy(){
		super.onDestroy();
		System.out.println("Termination");
	}
	
	@Override
	public void onStop(){
		super.onStop();
		System.out.println("Stop");
	}
	
	public class Thread_switch extends Thread {
		public void run(){
			int count;
			while(!Thread.currentThread().isInterrupted()){
				SystemClock.sleep(50);
				count = controlswitch();
				if(count == 0 && acc > 0) acc--;
				else if(count == 1 && acc < 200) acc++;
			}
		}
	}
	
	public class Thread_dot extends Thread {
		public void run(){
			int count = 0;
			int sign = 1;
			int fnd_data = 2200;
			int amount = 3800;
			
			while(!Thread.currentThread().isInterrupted()){
				SystemClock.sleep(205-acc);
				count += sign;
				controldotfnd(count, fnd_data);
				if(count == 0 || count == 10) sign *= -1;
				fnd_data -= 17;
				if(fnd_data < 0) {
					fnd_data = 1400;
					amount += 100;
				}
				controltextlcd(String.valueOf(acc), String.valueOf(amount));
			}
		}
	}
}


